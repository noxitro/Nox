//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	entity.h
///	@brief	entity
#pragma once
#include "dev_net_definition.h"
#include "../../object.h"

namespace nox
{
	class World;
}

namespace nox::dev::net
{
	std::expected<void, nox::dev::net::SocketIoError> ReceiveAll(
		nox::dev::net::raw_socket_t socket,
		nox::not_null<void*> buffer,
		nox::int32 size_to_read,
		nox::dev::net::ReceiveFlag flag = nox::dev::net::ReceiveFlag::None
	);

	std::expected<void, nox::dev::net::SocketIoError> SendAll(
		nox::dev::net::raw_socket_t socket,
		nox::not_null<const void*> buffer,
		nox::int32 size_to_send,
		nox::dev::net::SendFlag flag = nox::dev::net::SendFlag::None
	);

	class Entity : public nox::Object
	{
		NOX_DECLARE_OBJECT(nox::dev::net::Entity, nox::Object);
	public:
		Entity();
		virtual ~Entity();

		std::expected<void, nox::dev::net::SocketIoError> Receive(nox::dev::net::raw_socket_t socket, nox::not_null<void*> buffer, nox::int32 size_to_read, nox::dev::net::ReceiveFlag flag = nox::dev::net::ReceiveFlag::None);
		std::expected<void, nox::dev::net::SocketIoError> Send(nox::dev::net::raw_socket_t socket, nox::not_null<const void*> buffer, nox::int32 size_to_send, nox::dev::net::SendFlag flag = nox::dev::net::SendFlag::None);

		template<nox::concepts::TriviallyCopyable T>
		inline std::expected<void, nox::dev::net::SocketIoError> Send(nox::dev::net::raw_socket_t socket, const T& buffer, nox::dev::net::SendFlag flag = nox::dev::net::SendFlag::None)
		{
			return this->Send(socket, static_cast<const void*>(&buffer), static_cast<nox::int32>(sizeof(T)));
		}

	protected:
		virtual void OnSent() {}
		virtual void OnReceive([[maybe_unused]] nox::World& world) {}

	protected:
		static inline nox::dev::net::port_t base_port_ = 0;
	};
}