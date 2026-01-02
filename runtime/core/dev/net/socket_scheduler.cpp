//	Copyright (c) 2025 NOX ENGINE All rights reserved.

///	@file	socket_scheduler.cpp
///	@brief	socket_scheduler
#include	"stdafx.h"
#include	"socket_scheduler.h"

#include	"server.h"
#include	"client.h"
#include	"application.h"
namespace nox::util
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

void	nox::dev::net::SocketScheduler::Update()
{
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

		for (nox::dev::net::Server& server : server_list_)
		{
			server.PollAccept();
		}
	}
}

void	nox::dev::net::SocketScheduler::DoConnectionServerClient()
{
}

void	nox::dev::net::SocketScheduler::RegisterEntity(Server& entity)
{
	server_list_.emplace_back(entity);
}

void	nox::dev::net::SocketScheduler::RegisterEntity(Client&)
{
}

void	nox::dev::net::SocketScheduler::UnregisterEntity(nox::dev::net::Server& entity)
{
	const auto it = std::ranges::remove_if(server_list_,
		[&entity](const std::reference_wrapper<Server>& r)noexcept
		{
			return std::addressof(r.get()) == std::addressof(entity);
		});

	server_list_.erase(it.begin(), it.end());
}

void	nox::dev::net::SocketScheduler::UnregisterEntity(Client&)
{

}