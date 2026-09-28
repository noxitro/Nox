// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

/// @file	asset_manager.h
/// @brief	asset_manager
#pragma once
#include	"service.h"

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

	/// @brief		アセットの生成とロードを受け持つService。
	/// @details	OnInitialize で nox::Asset 派生型を拡張子で引く表を作ってロードスレッドを起こし、
	///				OnShutdown でロードスレッドを止めて join し、生成したアセットを破棄する。
	///
	///				毎フレームの処理(ノード)は持たない。CreateAsset はアセットをロードキューへ積んで返すだけで、
	///				ロード(ネイティブファイルの存在確認と nox::Asset::Initialize)はロードスレッドで完結する。
	///				完了はロードスレッドが nox::Asset の状態(IsReady)へ直接書き込む。フレームへ取り込むノードはまだ無い。
	///				ロードスレッドは World に触れない(Service は World への参照を持たない)。停止は自身の停止フラグで行う。
	///
	///				開発ビルドでは、CreateAsset がコンバート要求を Editor へ送るので EditorRemoteServer に依存する
	///				(Depends。初期化はこちらが後、終了はこちらが先)。Master ではその依存は無い。
	class AssetManager : public nox::Service
	{
		NOX_DECLARE_OBJECT(AssetManager, nox::Service);
	public:
#if NOX_DEVELOP
		/// @brief CreateAsset がコンバート要求を Editor へ送るので、EditorRemoteServer を先に初期化し、後に終了させる。
		/// @details EditorRemoteServer は開発ビルドにしか無いので、宣言も同じ条件で囲む。
		using Depends = nox::TypeList<nox::dev::editor_remote::EditorRemoteServer>;
#endif // NOX_DEVELOP

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
		/// @brief アセット型の表を作り、ロードスレッドを起こす。
		/// @details 開発ビルドでは Depends に並べた EditorRemoteServer を受け取る(引けなければ起動失敗)。
		bool OnInitialize(nox::ServiceContext& context)noexcept override;
		/// @brief ロードスレッドを止めて join し、生成したアセットを破棄する。
		/// @details 開発ビルドでは、依存先の EditorRemoteServer はまだ生きている。
		void OnShutdown()noexcept override;
		nox::Asset& CreateAssetImpl(std::u8string_view uri);

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

		/// @brief ロードスレッドの本体。停止フラグ(is_kill_)が立つまで回る。
		void LoadThread();

		/// @brief 単一アセットのロード処理（存在チェック・初期化チェック・初期化）
		/// @param asset 対象アセット
		/// @return 処理結果
		LoadStatus ProcessLoad(nox::Asset& asset);

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
		/// @brief ロードスレッドの停止要求。OnShutdown が立てる。
		std::atomic_bool is_kill_;
	};
}