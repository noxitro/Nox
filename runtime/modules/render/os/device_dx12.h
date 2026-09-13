//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	device_dx12.h
///	@brief	device_dx12
#pragma once
#include	"render_device.h"
#include	"os_dx12.h"

namespace nox::render
{
	struct GraphicBufferDesc;
	struct GraphicBufferSubResourceDesc;
	class VertexBuffer;
	class IndexBuffer;

	namespace command
	{
		class CommandBase;
	}
}

namespace nox::render::os
{
	class DeviceDX12 final: public nox::render::RenderDevice
	{
		NOX_DECLARE_OBJECT(nox::render::os::DeviceDX12, nox::render::RenderDevice);
	public:
		inline constexpr DeviceDX12()noexcept:
			device_(nullptr),
			swap_chain_(nullptr),
			command_list_(nullptr),
			command_allocator_(nullptr)
		{
		}

		inline constexpr ~DeviceDX12()noexcept override {}

		void Initialize()override;
		nox::IntrusivePtr<VertexBuffer> CreateVertexBuffer(const GraphicBufferDesc& desc, const GraphicBufferSubResourceDesc& sub_resource_desc)override;
	private:
		::ID3D12Device14* device_;
		::IDXGISwapChain4* swap_chain_;
		::ID3D12GraphicsCommandList10* command_list_;
		::ID3D12CommandAllocator* command_allocator_;
	};
}