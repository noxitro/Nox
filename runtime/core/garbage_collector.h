//	Copyright (C) 2024 NOX ENGINE All Rights Rserved.

///	@file	garbage_collector.h
///	@brief	garbage_collector
#pragma once
#include	"engine_system.h"

namespace nox
{
	class GarbageCollector : public nox::EngineSystem
	{
		NOX_DECLARE_OBJECT(nox::GarbageCollector, nox::EngineSystem);
	public:
		static void	Register(class nox::ManagedObject& managed_object);

	private:
		void Initialize(nox::Application& application);
		void FrameGC(nox::Application& application);
		void Finalize(nox::Application& application);

		std::span<const nox::EngineSystem::PhaseRegister> GetPhaseRegisterList()const noexcept override;
	public:
		static constexpr SystemPhaseUpdate k_phase_gc_update{
			&GarbageCollector::FrameGC,
			u8"GarbageCollector::FrameGC"
		};

		static constexpr SystemPhaseInit k_phase_init{
			&GarbageCollector::Initialize,
			u8"GarbageCollector::Initialize"
		};

		static constexpr SystemPhaseTerminate k_phase_terminal{
			&GarbageCollector::Finalize,
			u8"GarbageCollector::Finalize"
		};
	private:
		class Impl;
		static inline constinit Impl* impl_ = nullptr;
	};
}