// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

/// @file	asset_manager.cpp
/// @brief	asset_manager
#include "pch.h"
#include "asset_manager.h"
#include	"node_descriptor.h"

#include	"asset.h"
#include	"asset_ref.h"
#include	"core_utility.h"
#include	"asset_attribute.h"
#include	"log_id.h"

#if NOX_DEVELOP
#include	"dev/editor_remote_server.h"
#include	"dev/remote/remote_system.g.h"
#endif // NOX_DEVELOP


namespace nox
{
	namespace
	{
		/// @brief ネイティブコンバート待ちで再試行するまでの待機時間(ms)
		constexpr nox::uint32 k_load_retry_interval_ms = 100;

		inline nox::U8FixedString<nox::k_max_path_length> GetFullPath(std::u8string_view native_path)
		{
			//	
			std::array<nox::char8, nox::k_max_path_length> project_dir_buffer{};
			const std::u8string_view project_dir = nox::util::GetProjectDir(project_dir_buffer);

			std::array<std::byte, nox::k_max_path_length> full_path_buffer{};
			nox::SpanAllocStlU8String full_path_string(full_path_buffer);
			auto& native_full_path_pmr = full_path_string.GetContainer();
			native_full_path_pmr.append(project_dir);
			if (!native_full_path_pmr.empty() && native_full_path_pmr.back() != u8'/' && native_full_path_pmr.back() != u8'\\')
			{
				native_full_path_pmr.push_back(u8'/');
			}
			native_full_path_pmr.append(std::u8string_view(native_path));

			nox::U8FixedString<nox::k_max_path_length> native_full_path;
			native_full_path = std::u8string_view(native_full_path_pmr.data(), native_full_path_pmr.size());

			return native_full_path;
		}

		template<class String>
		void AppendCanonicalResourceRelativePath(String& dest, std::u8string_view relative_path)
		{
			// TODO: シーンリソース拡張子の正規化は未実装
			dest.append(relative_path);
		}
	}
}

namespace nox::util
{
	nox::U8FixedString<nox::k_max_path_length> nox::util::GetNativeResourcePath(std::u8string_view uri)
	{
		//	�v���W�F�N�g�f�B���N�g����native�p�X�����ɍs��
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

		std::array<std::byte, nox::k_max_path_length> buffer{};
		nox::SpanAllocStlU8String stack_string(buffer);
		auto& native_path = stack_string.GetContainer();

		native_path.append(u8"native/");
		native_path.append(nox::os::GetPlatformTypeName(nox::os::GetPlatformType()));
		native_path.push_back(u8'/');
		AppendCanonicalResourceRelativePath(native_path, relative_path);

		nox::U8FixedString<nox::k_max_path_length> result;
		result = std::u8string_view(native_path.data(), native_path.size());
		return result;
	}
}

const nox::NodeDescriptor& nox::AssetManager::GetNodeDescriptor()noexcept
{
	return node_descriptor_v<
		Register<kInit>,
#if NOX_DEVELOP
		Register<kUpdateSendConvertQuery,
		//nox::RunAfter<nox::dev::editor_remote::EditorRemoteServer::kReceive>
		>,
#endif // NOX_DEVELOP

		Register<kTerminate>
	>;
}

nox::AssetManager::AssetManager() :
	load_queue_signal_(0),
	is_kill_(false)
{

}

nox::AssetManager::~AssetManager()
{

}

void nox::AssetManager::Init()
{
	resource_cache_.clear();
	resource_typeinfo_map_with_extension_.clear();
	is_resource_class_cache_built_ = false;

	nox::reflection::ForeachDerivedClassInfoList<nox::Asset>([this](const nox::reflection::ClassInfo& class_info)
		{
			const nox::attr::Asset* const resource_attr = class_info.GetAttribute<nox::attr::Asset>();
			NOX_ASSERT(resource_attr != nullptr, u"Asset attribute not found:{0}", class_info.GetFullName());
			if (resource_attr == nullptr)
			{
				return;
			}

			const std::u8string_view extension = resource_attr->GetExtension();
			resource_typeinfo_map_with_extension_.emplace(extension, std::cref(class_info));
		});
	is_resource_class_cache_built_ = true;

	load_thread_.Dispatch([this]()
		{
			this->LoadThread();
		});
}

#if NOX_DEVELOP
void nox::AssetManager::UpdateSendConvertQuery(nox::dev::editor_remote::EditorRemoteServer& server)
{
	if (send_query_queue_.empty() == true)
	{
		return;
	}

	for (const auto& guid : send_query_queue_)
	{
		nox::Asset* const asset = resource_cache_.at(guid);
		if (asset == nullptr)
		{
			continue;
		}

		nox::dev::editor_remote::AssetConvertQuery query;
		//query.SetUri(asset->GetPath());
		//server.SendQuery(query);
	}

	send_query_queue_.clear();
	load_queue_signal_.release();
}
#endif // NOX_DEVELOP

void nox::AssetManager::Terminate()
{
	//	ロードスレッドを停止させる（停止フラグを立ててから起こし、終了を待つ）
	is_kill_.store(true);
	load_queue_signal_.release();
	load_thread_.Wait();

	for (auto& [path, resource] : resource_cache_)
	{
		(void)path;
		delete resource;
	}

	resource_cache_.clear();
	is_resource_class_cache_built_ = false;
}

nox::Asset& nox::AssetManager::CreateAssetImpl(const nox::detail::AssetHandleBase& handle, const nox::reflection::Type& type)const
{
	const nox::Guid guid = handle.GetGuid();
	//	キャッシュに存在する場合はそれを返す
	{
		NOX_LOCAL_SCOPE(nox::ScopedReadLock(rw_lock_));

		const auto it = resource_cache_.find(guid);
		if (it != resource_cache_.end())
		{
			return *it->second;
		}
	}

	nox::Asset* new_asset = nullptr;
	{

		NOX_LOCAL_SCOPE(nox::ScopedWriteLock(rw_lock_));

		// 共有ロックを外してから排他ロックを取るまでの間に、他のスレッドが足していた場合
		const auto it = resource_cache_.find(guid);
		if (it != resource_cache_.end())
		{
			
			return *it->second;
		}

		const nox::reflection::ClassInfo*const class_info = nox::reflection::FindClassInfo(type);
		NOX_ASSERT(class_info != nullptr, u8"class info not found");
		const nox::attr::Asset* const attr = class_info->GetAttribute<nox::attr::Asset>();
		NOX_ASSERT(attr != nullptr, u8"asset attribute not found");

		new_asset = static_cast<nox::Asset*>(type.CreateObject());
		resource_cache_.emplace(guid, new_asset);
	}

	{
		NOX_LOCAL_SCOPE(nox::ScopedWriteLock(load_queue_rw_lock_));
		load_queue_.push(new_asset);
		load_queue_signal_.release();
	}

	return *new_asset;
}

void nox::AssetManager::LoadThread()
{
	while (is_kill_.load() == false)
	{
		//	コンバートリクエスト(CreateAsset)をトリガーに起床する
		//	（複数積まれた場合の余分なトークンは、下のループで空キューを引いて消費される）
		load_queue_signal_.acquire();
		if (is_kill_.load() == true)
		{
			break;
		}

		//	キューが空になるまで、先頭のアセットを順に処理する
		while (is_kill_.load() == false)
		{
			//	先頭をピークする（popはしない）。キューへのアクセス時のみロックする
			nox::Asset* asset = nullptr;
			{
				NOX_LOCAL_SCOPE(nox::ScopedReadLock(load_queue_rw_lock_));
				if (load_queue_.empty() == true)
				{
					break;
				}
				asset = load_queue_.front();
			}

			const LoadStatus status = ProcessLoad(*asset);
			if (status == LoadStatus::Pending)
			{
				//	ネイティブコンバートがまだ完了していないので、先頭に残したまま少し待って再試行する
				nox::os::Sleep(k_load_retry_interval_ms);
				continue;
			}

			//	完了または破損したので先頭を取り除く
			NOX_LOCAL_SCOPE(nox::ScopedWriteLock(load_queue_rw_lock_));
			load_queue_.pop();
		}
	}
}

nox::AssetManager::LoadStatus nox::AssetManager::ProcessLoad(nox::Asset& asset)
{
	//const auto native_path = nox::util::GetNativeResourcePath(asset.GetPath());
	//const auto full_native_path = nox::GetFullPath(native_path);

	////	1. ファイルの存在チェック
	//if (nox::os::Exists(full_native_path) == false)
	//{
	//	//	ネイティブファイルがまだ生成されていない(コンバート待ち)
	//	return LoadStatus::Pending;
	//}

	////	2. 初期化チェック
	//{
	//	nox::io::File file;

	//	//	(a) ファイルを正常にオープンできるか(コンバート書き込み中はオープンに失敗し得る)
	//	if (file.Open(full_native_path, u8"rb") == false)
	//	{
	//		return LoadStatus::Pending;
	//	}

	//	//	(b) ファイルサイズが0でないか(0は破損扱い)
	//	if (file.GetSize() == 0)
	//	{
	//		NOX_ERROR_LINE(nox::log_id::Resource, u8"破損したアセットです(ファイルサイズ0):{0}", asset.GetPath());
	//		return LoadStatus::Corrupted;
	//	}
	//}

	////	3. 初期化処理(ネイティブコンバートが完了しているので初期化を呼び出す)
	//if (asset.Initialize(full_native_path) == false)
	//{
	//	NOX_ERROR_LINE(nox::log_id::Resource, u8"アセットの初期化に失敗しました:{0}", asset.GetPath());
	//	return LoadStatus::Corrupted;
	//}

	return LoadStatus::Completed;
}
