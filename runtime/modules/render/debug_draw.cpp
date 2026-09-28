// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

/// @file	debug_draw.cpp
/// @brief	debug_draw
#include "pch.h"
#include "debug_draw.h"

#if NOX_DEVELOP
#include	"renderer.h"

bool nox::render::debug::DebugDraw::OnInitialize(nox::ServiceContext& context)noexcept
{
	renderer_ = context.Get<nox::render::Renderer>();
	return renderer_ != nullptr;
}

void nox::render::debug::DebugDraw::OnShutdown()noexcept
{
	renderer_ = nullptr;
}

void nox::render::debug::DebugDraw::UpdateDraw()
{
}

void nox::render::debug::DebugDraw::DrawLine(const nox::Float3& start, const nox::Float3& end, nox::Color color, nox::render::debug::DebugDrawOption option)
{

}

#endif // NOX_DEVELOP
