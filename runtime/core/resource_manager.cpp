//	Copyright (c) 2025 NOX ENGINE All rights reserved.

///	@file	resource_manager.cpp
///	@brief	resource_manager
#include	"pch.h"
#include	"resource_manager.h"

#include	"resource.h"
#include	"world.h"
#include	"core_utility.h"

namespace nox::util
{
	nox::U8FixedString<nox::os::k_max_path_length> nox::util::GetNativeResourcePath(std::u8string_view uri)
	{
		//	プロジェクトディレクトリのnativeパスを見に行く
		//	uri:assets/aaa/bbb/ccc.ext
		//	native:native/{platform}/aaa/bbb/ccc.ext
		//	拡張子はversion
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
		native_path.append(relative_path);

		nox::U8FixedString<nox::os::k_max_path_length> result;
		result = std::u8string_view(native_path.data(), native_path.size());
		return result;
	}
}

namespace nox
{
	namespace
	{
		

		const nox::reflection::ClassInfo* FindResourceClassInfo(std::u8string_view extension)
		{
			nox::StlU8String full_name = u8"nox::";
			full_name.append(extension);
			full_name.append(u8"Resource");
			return nox::reflection::FindClassInfo(full_name);
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
	}
}

nox::Resource* nox::ResourceManager::GetResourceImpl(std::u8string_view uri)
{
	//	キャッシュチェック
	{
		NOX_LOCAL_SCOPE(nox::os::ScopedWriteLock(rw_lock_));
		{
			auto it = resource_cache_.find(uri);
			if (it != resource_cache_.end())
			{
				return it->second;
			}
		}

		
		//	第二拡張子を取得する。第一拡張子はファイルシステムの拡張子で、第二拡張子はリソースクラスを特定するためのもの
		{
			const std::u8string_view resource_extension = nox::filesystem::GetExtensions(uri, 1);
			auto it = resource_typeinfo_map_with_extension_.find(resource_extension);
			NOX_ASSERT(it != resource_typeinfo_map_with_extension_.end(), u"リソースクラスが見つかりませんでした: {0}", resource_extension);
			const nox::reflection::ClassInfo& class_info = it->second;
			nox::Resource* const resource = static_cast<nox::Resource*>(class_info.GetType().CreateObject());
		}
	}

	//	nativeフォルダ下のパス
	const nox::U8FixedString<nox::os::k_max_path_length> native_path = util::GetNativeResourcePath(uri);
}

void nox::ResourceManager::Initialize(nox::World& world)
{
	resource_cache_.clear();
	resource_typeinfo_map_with_extension_.clear();
	is_resource_class_cache_built_ = false;

	load_thread_.Dispatch([&world, this]()
		{
			this->ProcLoad(world);
		});

	//	nox::Resource継承クラスを収集
	nox::reflection::ForeachDerivedClassInfoList<nox::Resource>([this](const nox::reflection::ClassInfo& class_info)
		{
			const nox::attr::Resource*const resource_attr = class_info.GetAttribute<nox::attr::Resource>();
			NOX_ASSERT(resource_attr != nullptr, u"Resourceクラスにはnox::attr::Resource属性が必要です:{0}", class_info.GetFullName());

			const std::u8string_view extension = resource_attr->GetExtension();
			resource_typeinfo_map_with_extension_.emplace(extension, std::cref(class_info));
		});
}

void nox::ResourceManager::Finalize([[maybe_unused]] nox::World& world)
{
	for (auto& [path, resource] : resource_cache_)
	{
		(void)path;
		delete resource;
	}

	resource_cache_.clear();
	is_resource_class_cache_built_ = false;
}

void nox::ResourceManager::ProcLoad(nox::World& world)
{
	while (world.IsKill() == false)
	{
		{
			NOX_LOCAL_SCOPE(nox::os::ScopedReadLock(rw_lock_));
			
		}

		nox::os::Sleep(10);
	}
}

std::span<const nox::SystemBase::PhaseRegister> nox::ResourceManager::GetPhaseRegisterList()const noexcept
{
	static constexpr auto table = std::array{
		PhaseRegister(k_phase_init),
		PhaseRegister(k_phase_terminal)
	};
	return table;
}