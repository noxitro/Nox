//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	garbage_collector.h
///	@brief	garbage_collector
#pragma once
#include	"system.h"

namespace nox
{
	class GarbageCollector : public nox::SystemBase
	{
		NOX_DECLARE_OBJECT(nox::GarbageCollector, nox::SystemBase);
	public:
		static void	Register(class nox::Object& managed_object);

	private:
		void Initialize(nox::World& world);
		void FrameGC(nox::World& world);
		void Finalize(nox::World& world);

		std::span<const nox::SystemBase::PhaseRegister> GetPhaseRegisterList()const noexcept override;
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