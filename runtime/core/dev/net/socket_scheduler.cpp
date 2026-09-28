//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	socket_scheduler.cpp
///	@brief	socket_scheduler
#include	"pch.h"
#include	"socket_scheduler.h"

#include	"server.h"
#include	"client.h"
#include	"dev_net_log_id.h"

nox::dev::net::SocketScheduler::SocketScheduler() :
	server_list_(),
	pending_server_list_(),
	client_list_(),
	thread_(),
	mutex_server_list_(),
	stop_requested_(false),
	is_thread_running_(false),
	requested_list_revision_(0u),
	applied_list_revision_(0u),
	is_wsa_started_(false)
{
}

nox::dev::net::SocketScheduler::~SocketScheduler()
{
	//	OnShutdown を通らずに破棄される経路(起動途中の破棄など)でも、スレッドを止めてから解放する。
	//	OnShutdown 済みなら何もしない(Wait は起動していないスレッドに対しては何もしない)。
	stop_requested_.store(true, std::memory_order_release);
	thread_.Wait();
}

bool nox::dev::net::SocketScheduler::OnInitialize([[maybe_unused]] nox::ServiceContext& context)noexcept
{
#if NOX_WINDOWS
	::WSADATA wsaData;
	const nox::int32 error_code = ::WSAStartup(WINSOCK_VERSION, &wsaData);
	NOX_ASSERT(error_code == 0, nox::util::Format(u"WSAStartup failed. error_code={0}", error_code));
	if (error_code != 0)
	{
		//	開発用の通信が使えないだけなので、起動は止めない。受信スレッドも起こさない。
		NOX_ERROR_LINE(nox::dev::net::log_id::DevNet, u"WSAStartup failed. error_code={0}", error_code);
		return true;
	}
	is_wsa_started_ = true;
#endif // NOX_WINDOWS

	stop_requested_.store(false, std::memory_order_release);
	is_thread_running_.store(true, std::memory_order_release);
	thread_.SetThreadPriority(nox::os::ThreadPriority::Lowest);
	thread_.Dispatch([this]() {
		//	スレッド名はスレッドローカルなので、起こしたスレッドの中で付ける
		//	(呼び出し側で付けると、OnInitialize を呼んだスレッドの名前が変わる)。
		nox::os::Thread::SetThreadName(u"SocketScheduler");
		this->UpdateTask();
		});
	return true;
}

void nox::dev::net::SocketScheduler::OnShutdown()noexcept
{
	//	World::Exit から呼ばれる(旧 World::IsKill() による停止の代わり)。
	//	このServiceに依存するService(EditorRemoteServer など)は終了済みで、Server の登録も外れている。
	stop_requested_.store(true, std::memory_order_release);
	thread_.Wait();

#if NOX_WINDOWS
	if (is_wsa_started_)
	{
		const nox::int32 error_code = ::WSACleanup();
		NOX_ASSERT(error_code == 0, nox::util::Format(u"WSACleanup failed. error_code={0}", error_code));
		is_wsa_started_ = false;
	}
#endif // NOX_WINDOWS
}

void nox::dev::net::SocketScheduler::ApplyPendingServerListLocked()
{
	for (const auto& [server_ref, is_register] : pending_server_list_)
	{
		auto it = std::ranges::find_if(server_list_,
			[&server_ref](const std::reference_wrapper<Server>& r)noexcept
			{
				return std::addressof(r.get()) == std::addressof(server_ref.get());
			});

		if (is_register)
		{
			if (it == server_list_.end())
			{
				server_list_.emplace_back(server_ref);
			}
		}
		else if (it != server_list_.end())
		{
			server_list_.erase(it);
		}
	}

	pending_server_list_.clear();
	applied_list_revision_.store(requested_list_revision_.load(std::memory_order_acquire), std::memory_order_release);
}

void nox::dev::net::SocketScheduler::UpdateTask()
{
	while (stop_requested_.load(std::memory_order_acquire) == false)
	{
		//	保留リストから本リストへ移動。反映は必ずここ(周回の頭)で行うので、反映し終えた時点で
		//	外した Server には前の周回の処理も含めて触れていない(UnregisterEntity はこれを待つ)。
		if (applied_list_revision_.load(std::memory_order_acquire) != requested_list_revision_.load(std::memory_order_acquire))
		{
			NOX_LOCAL_SCOPE(nox::os::ScopedLock(mutex_server_list_));
			ApplyPendingServerListLocked();
		}

		//	サーバーリストがない場合は10ms待機
		if (server_list_.empty())
		{
			nox::os::Sleep(10);
			continue;
		}

		//	リッスンsocketの新規接続を監視
		{
			::fd_set fds;
			nox::os::file_descriptor::Zero(fds);

			for (nox::dev::net::Server& server : server_list_)
			{
				//	ソケット登録
				nox::os::file_descriptor::Set(server.GetSocket(), fds);
			}

			//	タイムアウト設定
			constexpr ::timeval timeout
			{
				.tv_sec = 0,
				.tv_usec = 1000	//	1ms
			};

			//MEMO:	selectの第一引数はwindowsでは無視される
			const auto select_result = ::select(0, &fds, nullptr, nullptr, &timeout);

			if (select_result == SOCKET_ERROR)
			{
				const int err = ::WSAGetLastError();
				NOX_ERROR_LINE(nox::dev::net::log_id::DevNet, u"select() failed. error_code={0}", err);
				continue;
			}
			else if (select_result == EINTR)
			{
				// シグナル受信によるselect終了の場合、再度待ち受けに戻る
				NOX_WARNING_LINE(nox::dev::net::log_id::DevNet, u"select() interrupted by signal.");
				continue;
			}

			for (nox::dev::net::Server& server : server_list_)
			{
				//	新規接続（リッスンソケットにイベントがあった場合のみ）
				if (select_result > 0)
				{
					server.Connection(fds);
				}

				//	受け付け処理
				server.Update();
			}
		}
	}

	//	以降このスレッドは Server に触れない。UnregisterEntity の待ちを解く。
	is_thread_running_.store(false, std::memory_order_release);
}

void	nox::dev::net::SocketScheduler::DoConnectionServerClient()
{
}

void	nox::dev::net::SocketScheduler::RegisterEntity(Server& entity)
{
	NOX_LOCAL_SCOPE(nox::os::ScopedLock(mutex_server_list_));
	pending_server_list_.emplace_back(std::make_tuple(std::ref(entity), true));
	requested_list_revision_.fetch_add(1u, std::memory_order_acq_rel);
}

void	nox::dev::net::SocketScheduler::RegisterEntity(Client&)
{
}

void	nox::dev::net::SocketScheduler::UnregisterEntity(nox::dev::net::Server& entity)
{
	nox::uint64 revision = 0u;
	{
		NOX_LOCAL_SCOPE(nox::os::ScopedLock(mutex_server_list_));
		pending_server_list_.emplace_back(std::make_tuple(std::ref(entity), false));
		revision = requested_list_revision_.fetch_add(1u, std::memory_order_acq_rel) + 1u;

		//	受信スレッドが動いていなければ、誰も反映しないのでその場で反映する。
		if (is_thread_running_.load(std::memory_order_acquire) == false)
		{
			ApplyPendingServerListLocked();
			return;
		}
	}

	//	受信スレッドが周回の頭で反映し終えるまで待つ。戻った後、受信スレッドはこの Server に触れない。
	//	終了時に1回呼ばれるだけなので、待ちは短いスリープで足りる(受信スレッドの1周は最長でも約10ms)。
	while ((is_thread_running_.load(std::memory_order_acquire) == true) &&
		(applied_list_revision_.load(std::memory_order_acquire) < revision))
	{
		nox::os::Sleep(1);
	}
}

void	nox::dev::net::SocketScheduler::UnregisterEntity(Client&)
{

}
