// Copyright (C) 2026 NOX ENGINE All rights reserved.

/// @file	asset_manager.h
/// @brief	asset_manager
#pragma once
#include	"system.h"

#if NOX_DEVELOP
namespace nox::dev::editor_remote
{
	class EditorRemoteServer;
}
#endif // NOX_DEVELOP

namespace nox
{
	namespace util
	{
		/// @brief uriをnative pathに変換する
		/// @param uri
		/// @return
		::nox::U8FixedString<nox::os::k_max_path_length> GetNativeResourcePath(std::u8string_view uri);
	}

	class Asset;

	class AssetManager : public nox::SystemBase
	{
		NOX_DECLARE_OBJECT(AssetManager, nox::SystemBase);
	public:
		AssetManager();
		~AssetManager()override;

		template<std::derived_from<nox::Asset> T>
		inline nox::IntrusivePtr<T> CreateAsset(std::u8string_view file_path)
		{
			nox::IntrusivePtr<T> resource;
			resource.Reset(static_cast<T*>(&CreateAssetImpl(file_path)));
			return resource;
		}

		inline nox::IntrusivePtr<nox::Asset> CreateAsset(std::u8string_view file_path)
		{
			nox::IntrusivePtr<nox::Asset> resource;
			resource.Reset(&CreateAssetImpl(file_path));
			return resource;
		}

	private:
		void Init(nox::World& world);
		void Terminate(nox::World& world);
		nox::Asset& CreateAssetImpl(std::u8string_view uri);

		std::span<const nox::SystemBase::PhaseRegister> GetPhaseRegisterList()const noexcept override;

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

		void LoadThread(nox::World& world);

		/// @brief 単一アセットのロード処理（存在チェック・初期化チェック・初期化）
		/// @param asset 対象アセット
		/// @return 処理結果
		LoadStatus ProcessLoad(nox::Asset& asset);
	public:
		static constexpr SystemPhaseInit kPhaseInit{
			&AssetManager::Init,
			u8"AssetManager::Init"
		};

		static constexpr SystemPhaseTerminate kPhaseTerminate{
			&AssetManager::Terminate,
			u8"AssetManager::Terminate"
		};

	private:
#if NOX_DEVELOP
		nox::util::InitOnceRef<nox::dev::editor_remote::EditorRemoteServer> editor_remote_server_system_;
#endif // NOX_DEVELOP

		nox::UnorderedMap<std::u8string_view, nox::Asset*> resource_cache_;
		nox::UnorderedMap<std::u8string_view, std::reference_wrapper<const nox::reflection::ClassInfo>> resource_typeinfo_map_with_extension_;
		nox::Queue<nox::Asset*> load_queue_;
		bool is_resource_class_cache_built_ = false;
		mutable nox::os::ReadWriteLock rw_lock_;
		nox::os::ReadWriteLock load_queue_rw_lock_;
		std::counting_semaphore<> load_queue_signal_;

		nox::os::Thread load_thread_;
		std::atomic_bool is_kill_;
	};
}