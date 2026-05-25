//	Copyright (C) 2023 NOX ENGINE All Rights Rserved.

///	@file	behavior.cpp
///	@brief	behavior
#include	"pch.h"
#include	"behavior.h"

#include	"behavior_manager.h"

std::span<const nox::SystemBase::PhaseRegister> nox::Behavior::GetPhaseRegisterList()const noexcept
{
	static constexpr auto table = {
		PhaseRegister(k_phase_update),
		PhaseRegister(k_phase_late_update)
	};
	return table;
}