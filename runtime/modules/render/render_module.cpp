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

void nox::render::RenderModule::RegisterServices(nox::World& world)const
{
	//	所有権は World に移る。初期化順は登録順ではなく Depends で決まる
	//	(DebugDraw は Renderer に依存するので、Renderer が先に初期化され、後に終了する)。
	world.RegisterService(*new nox::render::Renderer());
#if NOX_DEVELOP
	world.RegisterService(*new nox::render::debug::DebugDraw());
#endif // NOX_DEVELOP
}