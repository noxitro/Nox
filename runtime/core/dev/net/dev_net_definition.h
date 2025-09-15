//	Copyright (C) 2025 NOX ENGINE All rights reserved.

///	@file	dev_net_definition.h
///	@brief	dev_net_definition
#pragma once

namespace nox::dev::net
{
#if NOX_WINDOWS
	using port_t = nox::uint32;
	using raw_socket_t = ::SOCKET;

	struct address_t
	{
		constexpr nox::uint8* data() noexcept { return buffer; }
		inline constexpr const nox::uint8* data()const noexcept { return buffer; }

	private:
		nox::uint8 buffer[64];
	};

	struct peer_name_t
	{
		constexpr nox::uint8* data() noexcept { return buffer; }
		inline constexpr const nox::uint8* data()const noexcept { return buffer; }

	private:
		nox::uint8 buffer[64];
	};

#endif // NOX_WINDOWS
	using port_name_t = peer_name_t;

	struct ConnectionContext
	{
		nox::uint32 unique_id;
		address_t address;
		port_t port;
		peer_name_t peername;
		port_name_t portname;
	};
	using DisconnectionContext = ConnectionContext;

	struct PeerContext
	{
		ConnectionContext connection;
		raw_socket_t socket;
	};
}