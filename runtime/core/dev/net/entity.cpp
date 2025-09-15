//	Copyright (c) 2025 NOX ENGINE All rights reserved.

///	@file	entity.cpp
///	@brief	entity
#include	"stdafx.h"
#include	"entity.h"

::size_t nox::dev::net::Entity::recive(nox::dev::net::raw_socket_t socket,nox::not_null<void*> buffer,::size_t size_to_read,bool& disconnected)
{
	disconnected = false;

#if NOX_WINDOWS
	if (socket == INVALID_SOCKET)
	{
		disconnected = true;
		return 0;
	}

	int len = static_cast<int>(size_to_read > static_cast<::size_t>(INT_MAX) ? INT_MAX : size_to_read);
	int ret = ::recv(socket, static_cast<char*>(buffer.get()), len, 0);
	if (ret == 0)
	{
		// graceful disconnect
		disconnected = true;
		return 0;
	}
	if (ret == SOCKET_ERROR)
	{
		const int err = ::WSAGetLastError();
		// Non-fatal for non-blocking sockets
		if (err == WSAEWOULDBLOCK || err == WSAEINTR)
		{
			return 0;
		}
		// Treat other errors as disconnects
		switch (err)
		{
		case WSAECONNRESET:
		case WSAENETRESET:
		case WSAENETDOWN:
		case WSAESHUTDOWN:
		case WSAENOTCONN:
			disconnected = true;
			break;
		default:
			break;
		}
		return 0;
	}
	return static_cast<::size_t>(ret);
#else
	(void)socket; (void)buffer; (void)size_to_read; (void)disconnected;
	return 0;
#endif
}

::size_t nox::dev::net::Entity::send(nox::dev::net::raw_socket_t socket, nox::not_null<const void*> buffer, ::size_t size_to_send, bool& disconnected, bool non_aio)
{
	disconnected = false;

#if NOX_WINDOWS
	if (socket == INVALID_SOCKET)
	{
		disconnected = true;
		return 0;
	}

	int len = static_cast<int>(size_to_send > static_cast<::size_t>(INT_MAX) ? INT_MAX : size_to_send);
	int ret = ::send(socket, static_cast<const char*>(buffer.get()), len, 0);
	if (ret == SOCKET_ERROR)
	{
		const int err = ::WSAGetLastError();
		// Non-fatal for non-blocking sockets
		if (err == WSAEWOULDBLOCK || err == WSAEINTR)
		{
			return 0;
		}
		// Treat other errors as disconnects
		switch (err)
		{
		case WSAECONNRESET:
		case WSAENETRESET:
		case WSAENETDOWN:
		case WSAESHUTDOWN:
		case WSAENOTCONN:
			disconnected = true;
			break;
		default:
			break;
		}
		return 0;
	}
	return static_cast<::size_t>(ret);
#else
	(void)socket; (void)buffer; (void)size_to_send; (void)disconnected;
	return 0;
#endif
}

void nox::dev::net::Entity::onConnect(nox::dev::net::ConnectionContext& context)
{
	(void)context;
}

void nox::dev::net::Entity::onDisconnect(nox::dev::net::DisconnectionContext& context)
{
	(void)context;
}

void nox::dev::net::Entity::onSent(const nox::dev::net::PeerContext& context, nox::uint32 handle, nox::not_null<const void*> buffer, ::size_t size_to_send, ::size_t size_sent)
{
	(void)context; (void)handle; (void)buffer; (void)size_to_send; (void)size_sent;
}

void nox::dev::net::Entity::onRecive(const nox::dev::net::PeerContext& context)
{
	(void)context;
}