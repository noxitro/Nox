//	Copyright (C) 2025 NOX ENGINE All rights reserved.

///	@file	dev_net_api.h
///	@brief	dev_net_api
#pragma once
#include	"dev_net_definition.h"

namespace nox::dev::net
{
	inline	nox::int32	Bind(nox::dev::net::raw_socket_t socket, const nox::dev::net::raw_sockaddr& addr, const nox::int32 length = sizeof(nox::dev::net::raw_sockaddr))
	{
#if NOX_WINDOWS
		return ::bind(socket, &addr, length);
#else
		return -1;
#endif // NOX_WINDOWS
	}

	inline	nox::int32	Listen(nox::dev::net::raw_socket_t socket, const nox::int32 backlog = SOMAXCONN)
	{
#if NOX_WINDOWS
		return ::listen(socket, backlog);
#else
		return -1;
#endif // NOX_WINDOWS
	}

	inline	nox::dev::net::raw_socket_t	Accept(nox::dev::net::raw_socket_t socket, nox::dev::net::raw_sockaddr& addr, nox::int32& length)
	{
#if NOX_WINDOWS
		return ::accept(socket, &addr, &length);
#else
		return -1;
#endif // NOX_WINDOWS
	}

	inline	nox::int32	Send(nox::dev::net::raw_socket_t socket, const char* buffer, nox::int32 size_to_send, nox::int32 flag = 0)
	{
#if NOX_WINDOWS
		return ::send(socket, buffer, size_to_send, flag);
#else
		return -1;
#endif // NOX_WINDOWS
	}

	inline	nox::int32	Receive(nox::dev::net::raw_socket_t socket, char* buffer, nox::int32 size_to_read, nox::int32 flag = 0)
	{
#if NOX_WINDOWS
		return ::recv(socket, buffer, size_to_read, flag);
#else
		return -1;
#endif // NOX_WINDOWS
	}

	inline nox::int32 SetNoDelay(nox::dev::net::raw_socket_t socket, bool no_delay)
	{
#if NOX_WINDOWS
		const nox::int32 flag = no_delay ? 1 : 0;
		return ::setsockopt(socket, ::IPPROTO::IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char*>(&flag), sizeof(flag));
#else
		return -1;
#endif // NOX_WINDOWS
	}

	inline nox::int32 SetKeepAlive(nox::dev::net::raw_socket_t socket, bool keep_alive)
	{
#if NOX_WINDOWS
		const nox::int32 flag = keep_alive ? 1 : 0;
		return ::setsockopt(socket, SOL_SOCKET, SO_KEEPALIVE, reinterpret_cast<const char*>(&flag), sizeof(flag));
#else
		return -1;
#endif // NOX_WINDOWS
	}

	inline nox::int32 CloseSocket(nox::dev::net::raw_socket_t socket)
	{
#if NOX_WINDOWS
		return ::closesocket(socket);
#else
		return -1;
#endif
	}

	inline nox::int32 Shutdown(nox::dev::net::raw_socket_t socket, nox::int32 how /* SD_SEND/SD_RECEIVE/SD_BOTH */)
	{
#if NOX_WINDOWS
		return ::shutdown(socket, how);
#else
		return -1;
#endif
	}
}