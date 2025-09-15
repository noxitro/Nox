//	Copyright (C) 2025 NOX ENGINE All rights reserved.

///	@file	socket_scheduler.h
///	@brief	socket_scheduler
#pragma once
#include	"../../object.h"

namespace nox::dev::net
{
	class Server;
	class Client;

	class SocketScheduler : public nox::Object, public nox::ISingleton<SocketScheduler>
	{
		NOX_DECLARE_OBJECT(nox::dev::net::SocketScheduler, nox::Object);
	public:
		SocketScheduler();
		~SocketScheduler()override;

		void	Initialize();
		void	Update();
		void	Finalize();

		void	RegisterEntity(Server& entity);
		void	RegisterEntity(Client& entity);
		void	RegisterSocket(Server& entity);

		void	UnregisterEntity(Server& entity);
		void	UnregisterEntity(Client& entity);
		void	UnregisterSocket(Server& entity);
	private:
		void	UpdateTask();

	private:
		nox::Vector<std::reference_wrapper<Server>>	server_list_;
		nox::Vector<std::reference_wrapper<Client>>	client_list_;
	};
}