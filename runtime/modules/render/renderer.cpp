//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	renderer.cpp
///	@brief	renderer
#include	"pch.h"
#include	"renderer.h"

#include	"render_device.h"
#include	"detail/render_device_factory.h"

namespace nox::render
{
	
}

nox::render::Renderer::Renderer():
	render_device_(nox::render::detail::CreateRenderDevice())
{
}

nox::render::Renderer::~Renderer()
{
	delete render_device_;
	render_device_ = nullptr;
}

inline constexpr nox::render::RenderDevice& nox::render::Renderer::GetDevice()const noexcept
{
	return *render_device_;
}

void nox::render::Renderer::Init(nox::legacy::World& world)
{
}

void nox::render::Renderer::Update(nox::legacy::World& world)
{
}

void nox::render::Renderer::Terminate(nox::legacy::World& world)
{

}
