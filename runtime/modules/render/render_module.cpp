//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	render_module.cpp
///	@brief	render_module
#include	"pch.h"
#include	"render_module.h"

#include	"renderer.h"
#if NOX_DEVELOP
#include	"debug_draw.h"
#endif // NOX_DEVELOP


nox::render::RenderModule::RenderModule()
{
	
}

void nox::render::RenderModule::Register(nox::WorldRegistery& registry)const
{
	registry.Register<nox::render::Renderer>();
#if NOX_DEVELOP
//	registry.Register<nox::render::debug::DebugDraw>();
#endif // NOX_DEVELOP
}