//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	entity.h
///	@brief	entity
#pragma once
#include "dev_net_definition.h"
#include "../../object.h"

namespace nox::legacy
{
	class World;
}

namespace nox::dev::net
{
	std::expected<void, nox::dev::net::SocketIoError> ReceiveAll(
		nox::os::network::Socket socket,
		nox::not_null<void*> buffer,
		nox::int32 size_to_read,
		nox::os::network::ReceiveFlag flag = nox::os::network::ReceiveFlag::None
	);

	std::expected<void, nox::dev::net::SocketIoError> SendAll(
		nox::os::network::Socket socket,
		nox::not_null<const void*> buffer,
		nox::int32 size_to_send,
		nox::os::network::SendFlag flag = nox::os::network::SendFlag::None
	);

	class Entity : public nox::Object
	{
		NOX_DECLARE_OBJECT(nox::dev::net::Entity, nox::Object);
	public:
		Entity();
		virtual ~Entity();

		std::expected<void, nox::dev::net::SocketIoError> Receive(nox::os::network::Socket socket, nox::not_null<void*> buffer, nox::int32 size_to_read, nox::os::network::ReceiveFlag flag = nox::os::network::ReceiveFlag::None);
		std::expected<void, nox::dev::net::SocketIoError> Send(nox::os::network::Socket socket, nox::not_null<const void*> buffer, nox::int32 size_to_send, nox::os::network::SendFlag flag = nox::os::network::SendFlag::None);

		template<nox::concepts::TriviallyCopyable T>
		inline std::expected<void, nox::dev::net::SocketIoError> Send(nox::os::network::Socket socket, const T& buffer, nox::os::network::SendFlag flag = nox::os::network::SendFlag::None)
		{
			return this->Send(socket, static_cast<const void*>(&buffer), static_cast<nox::int32>(sizeof(T)), flag);
		}

	protected:
		virtual void OnSent() {}
		virtual void OnReceive([[maybe_unused]] nox::legacy::World& world) {}

	protected:
		static inline nox::dev::net::Port base_port_ = 0;
	};
}