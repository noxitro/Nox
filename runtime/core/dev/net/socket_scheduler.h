//	Copyright (C) 2025 NOX ENGINE All rights reserved.

///	@file	socket_scheduler.h
///	@brief	socket_scheduler
#pragma once
#include	"../../object.h"
#include	"dev_net_definition.h"

namespace nox::dev::net
{
	class Entity;
	class Server;
	class Client;

	class SocketScheduler : public nox::Object, public nox::ISingleton<SocketScheduler>
	{
		NOX_DECLARE_OBJECT(nox::dev::net::SocketScheduler, nox::Object);
		friend struct SocketSchedulerDetail;
	private:
		
	public:
		SocketScheduler();
		~SocketScheduler()override;

		void	Initialize();
		void	Update();
		void	Finalize();

		void	RegisterEntity(nox::dev::net::Server& entity);
		void	RegisterEntity(nox::dev::net::Client& entity);

		void	UnregisterEntity(nox::dev::net::Server& entity);
		void	UnregisterEntity(nox::dev::net::Client& entity);

	private:
		void	UpdateTask();
		void	DoConnectionServerClient();

	private:
		nox::Vector<std::reference_wrapper<Server>>	server_list_;
		nox::Vector<std::reference_wrapper<Client>>	client_list_;
	};
}