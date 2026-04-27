//	Copyright (C) 2025 NOX ENGINE All rights reserved.

///	@file	socket_scheduler.h
///	@brief	socket_scheduler
#pragma once
#include	"../../engine_system.h"
#include	"dev_net_definition.h"

namespace nox
{
	class Application;
}

namespace nox::dev::net
{
	class Entity;
	class Server;
	class Client;

	class SocketScheduler : public nox::EngineSystem
	{
		NOX_DECLARE_OBJECT(nox::dev::net::SocketScheduler, nox::EngineSystem);
		friend struct SocketSchedulerDetail;
	private:
		
	public:
		SocketScheduler();
		~SocketScheduler()override;

		void	RegisterEntity(nox::dev::net::Server& entity);
		void	RegisterEntity(nox::dev::net::Client& entity);

		void	UnregisterEntity(nox::dev::net::Server& entity);
		void	UnregisterEntity(nox::dev::net::Client& entity);

	private:
		void	Initialize(nox::Application& application);
		void	Finalize(nox::Application& application);

		void	UpdateTask(nox::Application& application);
		void	DoConnectionServerClient();

		std::span<const nox::EngineSystem::PhaseRegister>	GetPhaseRegisterList()const noexcept override;
	public:
		static constexpr SystemPhaseInit k_phase_init{
			&SocketScheduler::Initialize,
			NOX_U8_NAMEOF_FUNCTION(&SocketScheduler::Initialize)
		};

		static constexpr SystemPhaseUpdate k_phase_socket_update{
			&SocketScheduler::UpdateTask,
			NOX_U8_NAMEOF_FUNCTION(&SocketScheduler::UpdateTask)
		};

		static constexpr SystemPhaseTerminate k_phase_terminate{
			&SocketScheduler::Finalize,
			NOX_U8_NAMEOF_FUNCTION(&SocketScheduler::Finalize)
		};

	private:
		nox::Vector<std::reference_wrapper<Server>>	server_list_;
		nox::Vector<std::tuple<std::reference_wrapper<Server>, bool>> pending_server_list_;

		nox::Vector<std::reference_wrapper<Client>>	client_list_;

		nox::os::Thread thread_;
		nox::os::Mutex mutex_server_list_;
	};
}