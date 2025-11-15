//	Copyright (C) 2025 NOX ENGINE All rights reserved.

///	@file	server.h
///	@brief	server
#pragma once
#include	"entity.h"

namespace nox::dev::net
{
	class Server : public nox::dev::net::Entity
	{
		NOX_DECLARE_OBJECT(nox::dev::net::Server, nox::dev::net::Entity);
	public:
		struct InitializeContext
		{
			nox::uint16 max_connection;
			nox::dev::net::port_t port;
			bool resume_port;
		};

	private:
		enum class Phase : nox::uint8
		{
			None,
			Startuped,
		};

		enum class Flag : nox::uint8
		{
			None = 0x00,
			Startup = 0x01,
			Shutdown = 0x02,
			Error = 0x04,
		};

		struct PendingPeerContext
		{
			nox::dev::net::raw_socket_t  socket;
			ConnectionContext connection;
			ConnectionState connection_state = ConnectionState::Invalid;
		};
	public:
		~Server()override;
		bool Startup(const Server::InitializeContext& context);
		void Shutdown();

		inline bool IsStartup()const noexcept { return IsFlag(Flag::Startup); }
		void PollAccept();

		void Update();

	protected:
		void Connected(const nox::dev::net::ConnectionContext& context);
		virtual void OnConnected(const nox::dev::net::ConnectionContext& context) {}

		void Disconnected(const nox::dev::net::ConnectionContext& context);
		virtual void OnDisconnected(const nox::dev::net::ConnectionContext& context) {}
	private:
		inline	void ChangePhase(Phase phase);

	private:
		inline	void SetFlag(Flag flag, bool is_on)noexcept;
		inline	bool IsFlag(Flag flag)const noexcept;

	private:
		Server::InitializeContext initialize_context_;
		nox::Vector<PeerContext> client_list_;
		/// @brief 
		nox::Vector<nox::dev::net::raw_socket_t> detached_client_list_;
		Flag flags_;
		bool is_startup_;
		bool shutdown_;
		Phase phase_;
		bool is_error_;
		nox::dev::net::raw_socket_t socket_;
	};
}