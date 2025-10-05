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
		Entity();
		virtual ~Entity();

		::size_t Receive(nox::dev::net::raw_socket_t socket, nox::not_null<void*> buffer, ::size_t size_to_read, bool& disconnected);
		::size_t Send(nox::dev::net::raw_socket_t socket, nox::not_null<const void*> buffer, ::size_t size_to_send, bool& disconnected, bool non_aio = false);

	protected:
		void OnConnect(nox::dev::net::ConnectionContext& context);
		void OnDisconnect(nox::dev::net::DisconnectionContext& context);

		void OnSent(const nox::dev::net::PeerContext& context, nox::uint32 handle, nox::not_null<const void*> buffer, ::size_t size_to_send, ::size_t size_sent);
		void OnReceive(const nox::dev::net::PeerContext& context);

	protected:
		nox::os::Mutex mutex_;
		nox::dev::net::raw_socket_t socket_;
		static inline nox::dev::net::port_t base_port_ = 0;
	};
}