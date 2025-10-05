//	Copyright (C) 2025 NOX ENGINE All rights reserved.

///	@file	server.h
///	@brief	server
#pragma once
#include	"entity.h"

namespace nox::dev::net
{
	class Server : public nox::dev::net::Entity
	{
	public:
		struct InitializeContext
		{
			nox::uint16 max_connection;
			nox::dev::net::port_t port;
			bool resume_port;
		};

	public:
		~Server()override;
		void Startup(const Server::InitializeContext& context);
		void Shutdown();

	private:
		enum class Phase : nox::uint8
		{
			None,
			CheckNotAvail,
			Shutdown,
			Checkavail,
			Startup,
			End
		};

	private:
		Server::InitializeContext initialize_context_;
		nox::Vector<PeerContext> client_list_;
		nox::Vector<nox::dev::net::raw_socket_t> detached_client_list_;
		bool shutdown_;
		Phase phase_;
		bool is_error_;
	};
}