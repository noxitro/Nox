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

void nox::render::Renderer::Init(nox::World& world)
{
}

void nox::render::Renderer::Update(nox::World& world)
{
}

void nox::render::Renderer::Terminate(nox::World& world)
{

}

std::span<const nox::SystemBase::PhaseRegister> nox::render::Renderer::GetPhaseRegisterList()const noexcept
{
	static constexpr auto tbl = std::array{
		PhaseRegister(kPhaseInit),
		PhaseRegister(kPhaseUpdate),
		PhaseRegister(kPhaseTerminate)
	};
	return tbl;
}