//	Copyright (c) 2025 NOX ENGINE All rights reserved.

///	@file	socket_scheduler.cpp
///	@brief	socket_scheduler
#include	"stdafx.h"
#include	"socket_scheduler.h"

#include	"server.h"
#include	"client.h"
#include	"application.h"
#include	"dev_net_log_id.h"
namespace nox
{

	
}

namespace nox::dev::net
{
}

nox::dev::net::SocketScheduler::SocketScheduler()
{

}

nox::dev::net::SocketScheduler::~SocketScheduler()
{
}

void	nox::dev::net::SocketScheduler::Initialize()
{
#if NOX_WINDOWS
	::WSADATA wsaData;
	nox::dev::net::raw_socket_t sock = k_raw_invalid_socket;
	const nox::int32 error_code = ::WSAStartup(WINSOCK_VERSION, &wsaData);
	NOX_ASSERT(error_code == 0, nox::util::Format(u"WSAStartup failed. error_code={0}", error_code));
#endif // NOX_WINDOWS

	thread_.SetThreadName(u"SocketScheduler");
	thread_.SetThreadPriority(nox::os::ThreadPriority::Lowest);
	thread_.Dispatch([this]() {
		this->UpdateTask();
		});
}

void	nox::dev::net::SocketScheduler::Finalize()
{
#if NOX_WINDOWS
	const nox::int32 error_code = ::WSACleanup();
	NOX_ASSERT(error_code == 0, nox::util::Format(u"WSACleanup failed. error_code={0}", error_code));
#endif // NOX_WINDOWS

}

void	nox::dev::net::SocketScheduler::UpdateTask()
{
	while (true)
	{
		if (nox::Application::Instance().IsKill())
		{
			break;
		}

		//	保留リストから本リストへ移動
		{
			NOX_LOCAL_SCOPE(nox::os::ScopedLock(mutex_server_list_));

			for (const auto& [server_ref, is_register] : pending_server_list_)
			{
				if (is_register)
				{
					server_list_.emplace_back(server_ref);
				}
				else
				{
					auto it = std::ranges::find_if(server_list_,
						[&server_ref](const std::reference_wrapper<Server>& r)noexcept
						{
							return std::addressof(r.get()) == std::addressof(server_ref.get());
						});
					if (it != server_list_.end())
					{
						server_list_.erase(it);
					}
				}
			}
		}

		//	
		if (server_list_.empty())
		{
			nox::os::Sleep(1);
			continue;
		}

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
				.tv_usec = 5	//	1ms
			};

			//MEMO:	selectの第一引数はwindowsでは無視される
			const auto select_result = ::select(0, &fds, nullptr, nullptr, &timeout);
			if (select_result == 0)
			{
				nox::os::Sleep(1);
				continue;
			}
			
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
				//	接続待ち更新
				server.Connection(fds);

				//	受け付け処理
				server.Update(fds);
			}
		}
	}
}

void	nox::dev::net::SocketScheduler::DoConnectionServerClient()
{
}

void	nox::dev::net::SocketScheduler::RegisterEntity(Server& entity)
{
	NOX_LOCAL_SCOPE(nox::os::ScopedLock(mutex_server_list_));
	pending_server_list_.emplace_back(std::make_tuple(std::ref(entity), true));
}

void	nox::dev::net::SocketScheduler::RegisterEntity(Client&)
{
}

void	nox::dev::net::SocketScheduler::UnregisterEntity(nox::dev::net::Server& entity)
{
	NOX_LOCAL_SCOPE(nox::os::ScopedLock(mutex_server_list_));

	auto it = std::ranges::find_if(server_list_,
		[&entity](const std::reference_wrapper<Server>& r)noexcept
		{
			return std::addressof(r.get()) == std::addressof(entity);
		});

	if (it != server_list_.end())
	{
		pending_server_list_.emplace_back(std::make_tuple(std::ref(entity), false));
	}
}

void	nox::dev::net::SocketScheduler::UnregisterEntity(Client&)
{

}