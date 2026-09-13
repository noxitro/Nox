//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	render_device.h
///	@brief	render_device
#pragma once

namespace nox::render
{
	struct GraphicBufferDesc;
	struct GraphicBufferSubResourceDesc;
	class VertexBuffer;

	class RenderDevice : public nox::Object
	{
		NOX_DECLARE_OBJECT(nox::render::RenderDevice, nox::Object);
	public:
		inline RenderDevice() = default;
		inline ~RenderDevice()override = default;

		virtual void Initialize() = 0;
		virtual nox::IntrusivePtr<VertexBuffer> CreateVertexBuffer(const GraphicBufferDesc& desc, const GraphicBufferSubResourceDesc& sub_resource_desc) = 0;
	};
}