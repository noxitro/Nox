// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

/// @file	asset_manager.h
/// @brief	asset_manager
#pragma once
#include	"service.h"
#include	"ecs_definitions.h"

#if NOX_DEVELOP
namespace nox::dev::editor_remote
{
	class EditorRemoteServer;
}
#endif // NOX_DEVELOP

namespace nox
{
	namespace legacy
	{
		class World;
	}

	namespace util
	{
		/// @brief uriをnative pathに変換する
		/// @param uri
		/// @return
		::nox::U8FixedString<nox::k_max_path_length> GetNativeResourcePath(std::u8string_view uri);
	}

	class Asset;
	template<class>
	struct AssetHandle;
	namespace detail
	{
		struct AssetHandleBase;
	}

	class AssetManager final: public nox::Service<AssetManager>
	{
		NOX_ECS_DECLARE_VERIFY(AssetManager);
	public:
		AssetManager();
		~AssetManager();

		template<std::derived_from<nox::Asset> T>
		inline nox::IntrusivePtr<T> CreateAsset(const nox::AssetHandle<T>& handle)const
		{
			nox::IntrusivePtr<T> resource;
			resource.Reset(static_cast<T*>(&CreateAssetImpl(handle, nox::reflection::Typeof<T>())));
			return resource;
		}

	/*	inline nox::IntrusivePtr<nox::Asset> CreateAsset(nox::Guid guid)
		{
			nox::IntrusivePtr<nox::Asset> resource;
			resource.Reset(&CreateAssetImpl(file_path));
			return resource;
		}*/

	private:
		void Init();
#if NOX_DEVELOP
		void UpdateSendConvertQuery(nox::dev::editor_remote::EditorRemoteServer& server);
#endif // NOX_DEVELOP

		void Terminate();
		
		nox::Asset& CreateAssetImpl(const nox::detail::AssetHandleBase& handle, const nox::reflection::Type& type)const;

		/// @brief ロードスレッドでのアセット処理結果
		enum class LoadStatus : nox::uint8
		{
			/// @brief 初期化まで正常に完了した
			Completed,
			/// @brief ネイティブコンバートがまだ完了していないので再試行する
			Pending,
			/// @brief ファイルが破損しているので破棄する
			Corrupted,
		};

		void LoadThread();

		/// @brief 単一アセットのロード処理（存在チェック・初期化チェック・初期化）
		/// @param asset 対象アセット
		/// @return 処理結果
		LoadStatus ProcessLoad(nox::Asset& asset);
	public:
		static constexpr PhaseInit kInit{ &AssetManager::Init };
#if NOX_DEVELOP
		static constexpr PhaseUpdate kUpdateSendConvertQuery{ &AssetManager::UpdateSendConvertQuery };
#endif
		static constexpr PhaseTerminate kTerminate{ &AssetManager::Terminate };

		static const nox::NodeDescriptor& GetNodeDescriptor()noexcept;
	private:
		mutable nox::UnorderedMap<nox::Guid, nox::Asset*> resource_cache_;
		nox::UnorderedMap<std::u8string_view, std::reference_wrapper<const nox::reflection::ClassInfo>> resource_typeinfo_map_with_extension_;
		 mutable nox::Queue<nox::Asset*> load_queue_;

#if NOX_DEVELOP
		nox::Vector<nox::Guid> send_query_queue_;
#endif // NOX_DEVELOP


		bool is_resource_class_cache_built_ = false;
		mutable nox::ReadWriteLock rw_lock_;
		nox::ReadWriteLock load_queue_rw_lock_;
		mutable std::counting_semaphore<> load_queue_signal_;

		nox::Thread load_thread_;
		std::atomic_bool is_kill_;
	};
}