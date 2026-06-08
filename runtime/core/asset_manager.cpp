// Copyright (C) 2026 NOX ENGINE All rights reserved.

/// @file	asset_manager.cpp
/// @brief	asset_manager
#include "pch.h"
#include "asset_manager.h"

#include	"asset.h"
#include	"world.h"
#include	"core_utility.h"
#include	"asset_attribute.h"

#if NOX_DEVELOP
#include	"dev/editor_remote_server.h"
#include	"dev/remote/remote_system.g.h"
#endif // NOX_DEVELOP


namespace nox
{
	namespace
	{
		constexpr std::u8string_view k_scene_resource_extension = u8"noxscene";
		constexpr std::u8string_view k_legacy_scene_resource_extension = u8"scene";
		constexpr std::u8string_view k_short_scene_resource_extension = u8"scn";

		std::u8string_view NormalizeResourceExtension(std::u8string_view extension)noexcept
		{
			if (extension == k_legacy_scene_resource_extension || extension == k_short_scene_resource_extension)
			{
				return k_scene_resource_extension;
			}

			return extension;
		}

		inline nox::U8FixedString<nox::os::k_max_path_length> GetFullPath(std::u8string_view native_path)
		{
			//	ローカルのfullpath
			std::array<nox::char8, nox::os::k_max_path_length> project_dir_buffer{};
			const std::u8string_view project_dir = nox::util::GetProjectDir(project_dir_buffer);

			std::array<std::byte, nox::os::k_max_path_length> full_path_buffer{};
			nox::SpanAllocStlU8String full_path_string(full_path_buffer);
			auto& native_full_path_pmr = full_path_string.GetContainer();
			native_full_path_pmr.append(project_dir);
			if (!native_full_path_pmr.empty() && native_full_path_pmr.back() != u8'/' && native_full_path_pmr.back() != u8'\\')
			{
				native_full_path_pmr.push_back(u8'/');
			}
			native_full_path_pmr.append(std::u8string_view(native_path));

			nox::U8FixedString<nox::os::k_max_path_length> native_full_path;
			native_full_path = std::u8string_view(native_full_path_pmr.data(), native_full_path_pmr.size());

			return native_full_path;
		}

		template<class String>
		void AppendCanonicalResourceRelativePath(String& dest, std::u8string_view relative_path)
		{
			const std::u8string_view extension = NormalizeResourceExtension(nox::filesystem::GetExtensions(relative_path, 0));
			if (extension == k_scene_resource_extension)
			{
				const std::u8string_view source_extension = nox::filesystem::GetExtensions(relative_path, 0);
				const std::size_t base_length = relative_path.size() - source_extension.size();
				dest.append(relative_path.substr(0, base_length));
				dest.append(k_scene_resource_extension);
				return;
			}

			dest.append(relative_path);
		}
	}
}

namespace nox::util
{
	nox::U8FixedString<nox::os::k_max_path_length> nox::util::GetNativeResourcePath(std::u8string_view uri)
	{
		//	プロジェクトディレクトリのnativeパスを見に行く
		//	uri:assets/aaa/bbb/ccc.ext
		//	native:native/{platform}/aaa/bbb/ccc.ext
		std::u8string_view relative_path = uri;
		if (relative_path.starts_with(u8"assets:/"))
		{
			relative_path.remove_prefix(std::u8string_view(u8"assets:/").size());
		}
		else if (relative_path.starts_with(u8"assets/"))
		{
			relative_path.remove_prefix(std::u8string_view(u8"assets/").size());
		}

		while (!relative_path.empty() && (relative_path.front() == u8'/' || relative_path.front() == u8'\\'))
		{
			relative_path.remove_prefix(1);
		}

		std::array<std::byte, nox::os::k_max_path_length> buffer{};
		nox::SpanAllocStlU8String stack_string(buffer);
		auto& native_path = stack_string.GetContainer();

		native_path.append(u8"native/");
		native_path.append(nox::os::GetPlatformTypeName(nox::os::GetPlatformType()));
		native_path.push_back(u8'/');
		AppendCanonicalResourceRelativePath(native_path, relative_path);

		nox::U8FixedString<nox::os::k_max_path_length> result;
		result = std::u8string_view(native_path.data(), native_path.size());
		return result;
	}
}

nox::AssetManager::AssetManager() :
	is_kill_(false),
	load_queue_signal_(0)
{

}

nox::AssetManager::~AssetManager()
{

}

nox::Asset& nox::AssetManager::CreateAssetImpl(std::u8string_view uri)
{
	{
		//	リソースキャッシュに存在するか
		NOX_LOCAL_SCOPE(nox::os::ScopedReadLock(rw_lock_));

		const auto it = resource_cache_.find(uri);
		if (it != resource_cache_.end())
		{
			return *it->second;
		}
	}

	nox::Asset* new_asset = nullptr;
	{
		NOX_LOCAL_SCOPE(nox::os::ScopedWriteLock(rw_lock_));

		const auto it = resource_cache_.find(uri);
		if (it != resource_cache_.end())
		{
			
			return *it->second;
		}

		//	本当にないので作成する
		const std::u8string_view extension = NormalizeResourceExtension(nox::filesystem::GetExtensions(uri, 0));
		const nox::reflection::ClassInfo& class_info = resource_typeinfo_map_with_extension_.at(extension);
		new_asset = static_cast<nox::Asset*>(class_info.GetType().CreateObject());
		new_asset->Bind(uri, *this);

		resource_cache_.emplace(new_asset->GetPath(), new_asset);
	}

	//	ロードキューに追加
	{
		NOX_LOCAL_SCOPE(nox::os::ScopedWriteLock(load_queue_rw_lock_));
		load_queue_.push_back(new_asset);

		//	コンバートリクエスト
#if NOX_DEVELOP
		nox::dev::editor_remote::EditorRemoteServerSystem& server = editor_remote_server_system_;
#endif // NOX_DEVELOP

		//nox::dev::editor_remote::AssetConvertQuery
		load_queue_signal_.release();
	}

	return *new_asset;
}

void nox::AssetManager::Init(nox::World& world)
{
#if NOX_DEVELOP
	editor_remote_server_system_ = world.GetSystem<nox::dev::editor_remote::EditorRemoteServerSystem>();
#endif // NOX_DEVELOP


	resource_cache_.clear();
	resource_typeinfo_map_with_extension_.clear();
	is_resource_class_cache_built_ = false;

	//	nox::Asset継承クラスを収集
	nox::reflection::ForeachDerivedClassInfoList<nox::Asset>([this](const nox::reflection::ClassInfo& class_info)
		{
			const nox::attr::Asset* const resource_attr = class_info.GetAttribute<nox::attr::Asset>();
			NOX_ASSERT(resource_attr != nullptr, u"Assetクラスにはnox::attr::Asset属性が必要です:{0}", class_info.GetFullName());
			if (resource_attr == nullptr)
			{
				return;
			}

			const std::u8string_view extension = resource_attr->GetExtension();
			resource_typeinfo_map_with_extension_.emplace(extension, std::cref(class_info));
			if (extension == k_scene_resource_extension)
			{
				resource_typeinfo_map_with_extension_.emplace(k_legacy_scene_resource_extension, std::cref(class_info));
				resource_typeinfo_map_with_extension_.emplace(k_short_scene_resource_extension, std::cref(class_info));
			}
		});
	is_resource_class_cache_built_ = true;

	load_thread_.Dispatch([this, &world]()
		{
			this->LoadThread(world);
		});
}

void nox::AssetManager::Terminate([[maybe_unused]] nox::World& world)
{
	is_kill_ = true;

	for (auto& [path, resource] : resource_cache_)
	{
		(void)path;
		delete resource;
	}

	resource_cache_.clear();
	is_resource_class_cache_built_ = false;
}

std::span<const nox::SystemBase::PhaseRegister> nox::AssetManager::GetPhaseRegisterList()const noexcept
{
	static constexpr auto table = std::array{
		PhaseRegister(kPhaseInit),
		PhaseRegister(kPhaseTerminate)
	};
	return table;
}

void nox::AssetManager::LoadThread(nox::World& world)
{
	while (is_kill_ == false)
	{
		load_queue_signal_.acquire();
		NOX_LOCAL_SCOPE(nox::os::ScopedWriteLock(load_queue_rw_lock_));

		for (nox::Asset* asset : load_queue_)
		{
			//	ネイティブファイルの存在チェック
		}
	}
}