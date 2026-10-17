//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	entity.cpp
///	@brief	entity
#include	"pch.h"
#include	"net_entity.h"


namespace nox::dev::net
{
	/// @brief 送受信の最大サイズ
	constexpr nox::int32 k_max_size = 65536;

	namespace
	{
		inline nox::dev::net::SocketIoError ToSocketIoError(const nox::os::network::Error error) noexcept
		{
			switch (error)
			{
			case nox::os::network::Error::WouldBlock:
				return nox::dev::net::SocketIoError{ .kind = nox::dev::net::SocketIoError::WouldBlock, .network_error = error };
			case nox::os::network::Error::Interrupted:
				return nox::dev::net::SocketIoError{ .kind = nox::dev::net::SocketIoError::Interrupted, .network_error = error };
			default:
				return nox::dev::net::SocketIoError{ .kind = nox::dev::net::SocketIoError::Other, .network_error = error };
			}
		}
	}
}

nox::dev::net::Entity::Entity()
{
}

nox::dev::net::Entity::~Entity()
{
}

std::expected<void, nox::dev::net::SocketIoError> nox::dev::net::ReceiveAll(
	nox::os::network::Socket socket,
	nox::not_null<void*> buffer,
	nox::int32 size_to_read,
	nox::os::network::ReceiveFlag flag)
{
	NOX_ASSERT(size_to_read > 0 && size_to_read <= k_max_size, nox::util::Format(u"size_to_read is zero or too large. size:{0}", size_to_read));

	if (socket.IsValid() == false)
	{
		return std::unexpected(SocketIoError{ .kind = nox::dev::net::SocketIoError::InvalidSocket });
	}

	//	受け取りきる
	nox::uint8* const dest = static_cast<nox::uint8*>(buffer.get());
	nox::int32 remain_size = size_to_read;
	while (remain_size > 0)
	{
		const nox::os::network::IoResult result = nox::os::network::Receive(
			socket,
			std::span<nox::uint8>(dest + (size_to_read - remain_size), static_cast<std::size_t>(remain_size)),
			flag);
		if (result.IsOk() == false)
		{
			return std::unexpected(nox::dev::net::ToSocketIoError(result.error));
		}
		if (result.bytes == 0)
		{
			// 切断
			return std::unexpected(SocketIoError{ .kind = nox::dev::net::SocketIoError::Disconnected });
		}
		remain_size -= result.bytes;
	}

	return {};
}

std::expected<void, nox::dev::net::SocketIoError> nox::dev::net::SendAll(
	nox::os::network::Socket socket,
	nox::not_null<const void*> buffer,
	nox::int32 size_to_send,
	nox::os::network::SendFlag flag)
{
	NOX_ASSERT(size_to_send > 0 && size_to_send <= k_max_size, nox::util::Format(u"size_to_send is zero or too large. size:{0}", size_to_send));
	if (socket.IsValid() == false)
	{
		return std::unexpected(SocketIoError{ .kind = nox::dev::net::SocketIoError::InvalidSocket });
	}
	//	送りきる
	const nox::uint8* const source = static_cast<const nox::uint8*>(buffer.get());
	nox::int32 remain_size = size_to_send;
	while (remain_size > 0)
	{
		const nox::os::network::IoResult result = nox::os::network::Send(
			socket,
			std::span<const nox::uint8>(source + (size_to_send - remain_size), static_cast<std::size_t>(remain_size)),
			flag);
		if (result.IsOk() == false)
		{
			return std::unexpected(nox::dev::net::ToSocketIoError(result.error));
		}
		if (result.bytes == 0)
		{
			// 切断
			return std::unexpected(SocketIoError{ .kind = nox::dev::net::SocketIoError::Disconnected });
		}
		remain_size -= result.bytes;
	}
	return {};
}

std::expected<void, nox::dev::net::SocketIoError> nox::dev::net::Entity::Receive(
	nox::os::network::Socket socket,
	nox::not_null<void*> buffer,
	nox::int32 size_to_read,
	nox::os::network::ReceiveFlag flag)
{
	return nox::dev::net::ReceiveAll(socket, buffer, size_to_read, flag);
}

std::expected<void, nox::dev::net::SocketIoError> nox::dev::net::Entity::Send(
	nox::os::network::Socket socket,
	nox::not_null<const void*> buffer,
	nox::int32 size_to_send,
	nox::os::network::SendFlag flag)
{
	return nox::dev::net::SendAll(socket, buffer, size_to_send, flag);
}