//	Copyright (c) 2025 NOX ENGINE All rights reserved.

///	@file	entity.cpp
///	@brief	entity
#include	"stdafx.h"
#include	"entity.h"

#include	"dev_net_api.h"

namespace nox::dev::net
{
	/// @brief 送受信の最大サイズ
	constexpr nox::int32 k_max_size = 65536;
}

nox::dev::net::Entity::Entity()
{
}

nox::dev::net::Entity::~Entity()
{
}

std::expected<void, nox::dev::net::SocketIoError> nox::dev::net::Entity::Receive(nox::dev::net::raw_socket_t socket, nox::not_null<void*> buffer, nox::int32 size_to_read, nox::dev::net::ReceiveFlag flag)
{
	NOX_ASSERT(size_to_read > 0 && size_to_read <= k_max_size, nox::util::Format(u"size_to_read is zero or too large. size:{0}", size_to_read));

	if (socket == k_raw_invalid_socket)
	{
		return std::unexpected(SocketIoError{ .kind = nox::dev::net::SocketIoError::InvalidSocket });
	}

	//	受け取りきる
	nox::int32 remain_size = size_to_read;
	while (remain_size > 0)
	{
		nox::int32 recived = nox::dev::net::Receive(socket, static_cast<char*>(buffer.get()) + (size_to_read - remain_size), remain_size, nox::util::ToUnderlying(flag));
		if (recived > 0)
		{
			remain_size -= recived;
		}
		else if (recived == 0)
		{
			// 切断
			return std::unexpected(SocketIoError{ .kind = nox::dev::net::SocketIoError::Disconnected });
		}
		else
		{
			const nox::int32 err = ::WSAGetLastError();
			// その他エラー処理
			return std::unexpected(SocketIoError{ .kind = SocketIoError::Other, .platform_code = err });
		}
	}

	return {};
}

std::expected<void, nox::dev::net::SocketIoError> nox::dev::net::Entity::Send(nox::dev::net::raw_socket_t socket, nox::not_null<const void*> buffer, nox::int32 size_to_send, nox::dev::net::SendFlag flag)
{
	NOX_ASSERT(size_to_send > 0 && size_to_send <= k_max_size, nox::util::Format(u"size_to_send is zero or too large. size:{0}", size_to_send));
	if (socket == k_raw_invalid_socket)
	{
		return std::unexpected(SocketIoError{ .kind = nox::dev::net::SocketIoError::InvalidSocket });
	}
	//	送りきる
	nox::int32 remain_size = size_to_send;
	while (remain_size > 0)
	{
		nox::int32 sent = nox::dev::net::Send(socket, static_cast<const char*>(buffer.get()) + (size_to_send - remain_size), remain_size, nox::util::ToUnderlying(flag));
		if (sent > 0)
		{
			remain_size -= sent;
		}
		else if (sent == 0)
		{
			// 切断
			return std::unexpected(SocketIoError{ .kind = nox::dev::net::SocketIoError::Disconnected });
		}
		else
		{
			const nox::int32 err = ::WSAGetLastError();
			// その他エラー処理
			return std::unexpected(SocketIoError{ .kind = SocketIoError::Other, .platform_code = err });
		}
	}
	return {};
}