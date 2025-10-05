//	Copyright (c) 2025 NOX ENGINE All rights reserved.

///	@file	server.cpp
///	@brief	server
#include	"stdafx.h"
#include	"server.h"

#include	"socket_scheduler.h"

nox::dev::net::Server::~Server()
{
	nox::dev::net::SocketScheduler::Instance().UnregisterEntity(*this);
}

void nox::dev::net::Server::Startup(const InitializeContext& context)
{
	if (!shutdown_)
	{
		this->Shutdown();
	}

	nox::dev::net::SocketScheduler::Instance().RegisterEntity(*this);

	initialize_context_ = context;
	shutdown_ = false;
	phase_ = Phase::None;
	is_error_ = false;

#if NOX_WINDOWS
	::sockaddr_in ip_address;
	ip_address.sin_family = AF_INET;
	ip_address.sin_addr.S_un.S_addr = INADDR_ANY;
	ip_address.sin_port = ::htons(static_cast<nox::uint16>(context.port));
	this->socket_ = ::socket(ip_address.sin_family, SOCK_STREAM, 0);

	NOX_ASSERT(socket_ != INVALID_SOCKET, nox::util::Format(U"socket() failed. error_code={0}", ::WSAGetLastError()));

	//	非ブロッキング
	::u_long nb = 1;
	if(::ioctlsocket(this->socket_, FIONBIO, &nb) == SOCKET_ERROR)
	{
		NOX_ASSERT(false, nox::util::Format(U"ioctlsocket() failed. error_code={0}", ::WSAGetLastError()));
	}


	
#endif // NOX_WINDOWS

}

void nox::dev::net::Server::Shutdown()
{

}