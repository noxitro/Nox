//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	socket_scheduler.h
///	@brief	socket_scheduler
#pragma once
#include	"../../service.h"
#include	"dev_net_definition.h"
//	RegisterEntity / UnregisterEntity のリフレクション生成コードは引数の完全型を要求する。
//	前方宣言だけだと、editor_remote_server.h (NOX_DEVELOP 限定) 経由で server.h / client.h が
//	来ない Master 構成で不完全型のまま参照されて壊れるので、ここで直接取り込む。
#include	"server.h"
#include	"client.h"

namespace nox::dev::net
{
	/// @brief		ソケットの受信を専用スレッドで回すService。
	/// @details	OnInitialize で WinSock を初期化して受信スレッドを起こし、OnShutdown で止めて join する。
	///				受信スレッドは登録された nox::dev::net::Server の新規接続と受信を監視し、
	///				イベントハンドラ(nox::dev::net::IServerEventHandler)へ通知する。
	///
	///				毎フレームの処理(ノード)は持たない。受信スレッドはフェーズと無関係に回り続けるので、
	///				受信したデータをフレームへ取り込むのは Server の持ち主(EditorRemoteServer の FrameIngress のメソッドなど)の役目。
	///				受信スレッドは World に触れない(Serviceは World への参照を持たない)。
	///				停止は World::IsKill() ではなく、自身の停止フラグ(OnShutdown が立てる)で行う。
	///
	///				Server を使う Service は `using Depends = nox::TypeList<nox::dev::net::SocketScheduler>;` を宣言する。
	///				初期化はこのServiceが先、終了は後(Depends の逆順)になるので、
	///				使う側は OnShutdown で UnregisterEntity してから自分の Server を閉じればよい。
	class SocketScheduler : public nox::Service
	{
		NOX_DECLARE_OBJECT(nox::dev::net::SocketScheduler, nox::Service);
		friend struct SocketSchedulerDetail;
	private:
		struct Impl;
	public:
		SocketScheduler();
		~SocketScheduler()override;

		/// @brief 受信スレッドの監視対象に加える。反映は受信スレッドの次の周回の頭。
		void	RegisterEntity(nox::dev::net::Server& entity);
		void	RegisterEntity(nox::dev::net::Client& entity);

		/// @brief 受信スレッドの監視対象から外す。
		/// @details 受信スレッドが外し終えるまで待ってから戻る。戻った後、受信スレッドはこの Server に触れないので、
		///          呼び出し側はそのまま Server を閉じてよい。受信スレッドが動いていなければその場で外す。
		void	UnregisterEntity(nox::dev::net::Server& entity);
		void	UnregisterEntity(nox::dev::net::Client& entity);

	private:
		/// @brief WinSock を初期化し、受信スレッドを起こす。
		/// @details WinSock の初期化に失敗しても起動は止めない(開発用の通信が使えないだけで、ゲームは動かせる)。
		///          その場合は受信スレッドを起こさない。
		bool	OnInitialize(nox::ServiceContext& context)noexcept override;

		/// @brief 受信スレッドを止めて join し、WinSock を片付ける。
		/// @details このServiceに依存するService(Depends に並べた側)は終了済み。
		void	OnShutdown()noexcept override;

		/// @brief 受信スレッドの本体。停止フラグが立つまで回る。
		void	UpdateTask();
		void	DoConnectionServerClient();

		/// @brief 保留中の登録 / 解除を server_list_ へ反映する。mutex_server_list_ を取った状態で呼ぶ。
		void	ApplyPendingServerListLocked();

	private:
		nox::Vector<std::reference_wrapper<Server>>	server_list_;
		nox::Vector<std::tuple<std::reference_wrapper<Server>, bool>> pending_server_list_;

		nox::Vector<std::reference_wrapper<Client>>	client_list_;

		nox::os::Thread thread_;
		nox::os::Mutex mutex_server_list_;

		/// @brief 受信スレッドの停止要求。OnShutdown が立てる(World::IsKill() の代わり)。
		NOX_ATTR(nox::reflection::attr::IgnoreReflection())
		std::atomic<bool> stop_requested_;

		/// @brief 受信スレッドが動いているか。UnregisterEntity が反映を待つかどうかの判定に使う。
		NOX_ATTR(nox::reflection::attr::IgnoreReflection())
		std::atomic<bool> is_thread_running_;

		/// @brief 保留リストへ積んだ回数(mutex_server_list_ の中で進める)。
		NOX_ATTR(nox::reflection::attr::IgnoreReflection())
		std::atomic<nox::uint64> requested_list_revision_;

		/// @brief 受信スレッドが保留リストを反映し終えた時点の requested_list_revision_。
		NOX_ATTR(nox::reflection::attr::IgnoreReflection())
		std::atomic<nox::uint64> applied_list_revision_;

		/// @brief WSAStartup が成功したか。成功したときだけ OnShutdown で WSACleanup する。
		bool is_wsa_started_;
	};
}