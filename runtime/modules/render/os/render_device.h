//	Copyright (C) 2026 NOX ENGINE All rights reserved.

///	@file	render_device.h
///	@brief	render_device
#pragma once

namespace nox::render
{
	class RenderDevice : public nox::Object
	{
		NOX_DECLARE_OBJECT(nox::render::RenderDevice, nox::Object);
	public:
		inline RenderDevice() = default;
		inline ~RenderDevice()override = default;
	};
}