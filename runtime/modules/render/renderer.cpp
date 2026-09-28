//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	renderer.cpp
///	@brief	renderer
#include	"pch.h"
#include	"renderer.h"

#include	"os/device_dx12.h"
#include	"definition.h"

namespace nox::render
{
	
}

nox::render::Renderer::Renderer():
	render_device_(
#if NOX_RENDER_DX12
		new nox::render::os::DeviceDX12()
#else
		nullptr
#endif // NOX_RENDER_DX12
	)
{
}

nox::render::Renderer::~Renderer()
{
	delete render_device_;
	render_device_ = nullptr;
}

inline constexpr auto& nox::render::Renderer::GetDevice()const noexcept
{
#if NOX_RENDER_DX12
	return *static_cast<nox::render::os::DeviceDX12*>(render_device_);
#else
	static_assert(false, "Not implemented");
	return *render_device_;
#endif // NOX_RENDER_DX12

}

bool nox::render::Renderer::OnInitialize([[maybe_unused]] nox::ServiceContext& context)noexcept
{
	return true;
}

void nox::render::Renderer::OnShutdown()noexcept
{
}

void nox::render::Renderer::Update()
{
}