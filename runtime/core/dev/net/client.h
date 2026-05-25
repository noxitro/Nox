//	Copyright (C) 2025 NOX ENGINE All rights reserved.

///	@file	client.h
///	@brief	client
#pragma once
#include	"net_entity.h"
namespace nox::dev::net
{
	/*enum class ConnectionState : nox::uint8
	{
		Init,
		OpenConnection,
		WaitForConnection,
		HandShake1,
		HandShake2,
		HandShake3,
		Connectiong,
		COnnected,
		ConnectionError,
		Disconnected,
		DisconnectedRecover
	};*/

	class Client : public nox::dev::net::Entity
	{
		NOX_DECLARE_OBJECT(nox::dev::net::Client, nox::dev::net::Entity);
	public:
		struct InitializeContext
		{
			nox::dev::net::address_t address;
			nox::dev::net::port_t port;
		};

		void Initialize(const InitializeContext& context);

		inline constexpr void SetState(ConnectionState state) noexcept { state_ = state; }
		inline constexpr ConnectionState GetState()const noexcept { return state_; }


	private:
		InitializeContext initialize_context_;
		nox::dev::net::PeerContext server_context_;
		ConnectionState state_;
	};
}