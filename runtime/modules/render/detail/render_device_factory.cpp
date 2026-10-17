//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	render_device_factory.cpp
///	@brief	render_device_factory
#include	"pch.h"
#include	"render_device_factory.h"

#include	"../definition.h"
#include	"../render_device.h"
#if NOX_RENDER_DX12
#include	"device_dx12.h"
#endif // NOX_RENDER_DX12

nox::render::RenderDevice* nox::render::detail::CreateRenderDevice()
{
#if NOX_RENDER_DX12
	return new nox::render::detail::DeviceDX12();
#else
	return nullptr;
#endif // NOX_RENDER_DX12
}
