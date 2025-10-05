//	Copyright (c) 2025 NOX ENGINE All rights reserved.

///	@file	socket_scheduler.cpp
///	@brief	socket_scheduler
#include	"stdafx.h"
#include	"socket_scheduler.h"

#include	"server.h"
#include	"client.h"

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
	nox::dev::net::raw_socket_t sock = INVALID_SOCKET;
	const nox::int32 error_code = ::WSAStartup(WINSOCK_VERSION, &wsaData);
	NOX_ASSERT(error_code == 0, nox::util::Format(U"WSAStartup failed. error_code={0}", error_code));
#endif // NOX_WINDOWS

}

void	nox::dev::net::SocketScheduler::Update()
{
	UpdateTask();
}

void	nox::dev::net::SocketScheduler::Finalize()
{
#if NOX_WINDOWS
	const nox::int32 error_code = ::WSACleanup();
	NOX_ASSERT(error_code == 0, nox::util::Format(U"WSACleanup failed. error_code={0}", error_code));
#endif // NOX_WINDOWS

}

void	nox::dev::net::SocketScheduler::UpdateTask()
{
	CheckConnectionServerClient();
}

void	nox::dev::net::SocketScheduler::CheckConnectionServerClient()
{
	
}

void	nox::dev::net::SocketScheduler::RegisterEntity(Server& entity)
{
	server_list_.emplace_back(entity);
}

void	nox::dev::net::SocketScheduler::RegisterEntity(Client& entity)
{
}

void	nox::dev::net::SocketScheduler::UnregisterEntity(nox::dev::net::Server& entity)
{
	// 真に削除したいのは server_list_ (メンバ)
//	auto sub = std::ranges::remove(server_list_, entity); // Serverにoperator==が必要
//	server_list_.erase(sub.begin(), sub.end());
}

void	nox::dev::net::SocketScheduler::UnregisterEntity(Client& entity)
{

}