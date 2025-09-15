//	Copyright (C) 2025 NOX ENGINE All rights reserved.

///	@file	entity.h
///	@brief	entity
#pragma once
#include "dev_net_definition.h"

namespace nox::dev::net
{
	class Entity
	{
	public:
		::size_t recive(nox::dev::net::raw_socket_t socket, nox::not_null<void*> buffer, ::size_t size_to_read, bool& disconnected);
		::size_t send(nox::dev::net::raw_socket_t socket, nox::not_null<const void*> buffer, ::size_t size_to_send, bool& disconnected, bool non_aio = false);

	protected:
		void onConnect(nox::dev::net::ConnectionContext& context);
		void onDisconnect(nox::dev::net::DisconnectionContext& context);

		void onSent(const nox::dev::net::PeerContext& context, nox::uint32 handle, nox::not_null<const void*> buffer, ::size_t size_to_send, ::size_t size_sent);
		void onRecive(const nox::dev::net::PeerContext& context);

	private:
		nox::os::Mutex mutex_;
		nox::dev::net::raw_socket_t socket_;
		static inline nox::dev::net::port_t base_port_ = 0;
	};
}