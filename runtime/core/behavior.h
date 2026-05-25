//	Copyright (C) 2023 NOX ENGINE All Rights Rserved.

///	@file	behavior.h
///	@brief	behavior
#pragma once
#include	"component.h"
#include	"system.h"

namespace nox
{
	class Behavior : public nox::SystemBase
	{
		NOX_DECLARE_OBJECT(Behavior, nox::SystemBase);

	protected:
		void Update([[maybe_unused]] nox::World& world) {}
		void LateUpdate([[maybe_unused]] nox::World& world) {}

		std::span<const PhaseRegister> GetPhaseRegisterList()const noexcept override;

	private:
		static constexpr SystemPhaseUpdate k_phase_update{
			&Behavior::Update,
			u8"Behavior::Update"
		};

		static constexpr SystemPhaseUpdate k_phase_late_update{
			&Behavior::LateUpdate,
			u8"Behavior::LateUpdate"
		};
	};
}