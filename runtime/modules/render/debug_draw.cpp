// Copyright (C) 2026 NOX ENGINE All rights reserved.

/// @file	debug_draw.cpp
/// @brief	debug_draw
#include "pch.h"
#include "debug_draw.h"

#if NOX_DEVELOP
#include	"renderer.h"

void nox::render::debug::DebugDraw::Init(nox::World& world)
{
}

void nox::render::debug::DebugDraw::UpdateDraw(nox::World& world)
{
}

void nox::render::debug::DebugDraw::Terminate(nox::World& world)
{
}

std::span<const nox::SystemBase::PhaseRegister> nox::render::debug::DebugDraw::GetPhaseRegisterList()const noexcept
{
	static constexpr auto table = std::array{
		PhaseRegister{
			DebugDraw::kPhaseInit,
		{	nox::render::Renderer::kPhaseInit } },

		PhaseRegister{ DebugDraw::kPhaseUpdate,
		{},
		{nox::render::Renderer::kPhaseUpdate } },

		PhaseRegister{ DebugDraw::kPhaseTerminate,
		{nox::render::Renderer::kPhaseTerminate } }
	};
	return table;
}

void nox::render::debug::DebugDraw::DrawLine(const nox::Float3& start, const nox::Float3& end, nox::Color color, nox::render::debug::DebugDrawOption option)
{

}

#endif // NOX_DEVELOP
