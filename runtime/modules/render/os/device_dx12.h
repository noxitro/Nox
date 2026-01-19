//	Copyright (C) 2026 NOX ENGINE All rights reserved.

///	@file	device_dx12.h
///	@brief	device_dx12
#pragma once
#include	"render_device.h"
#include	"os_dx12.h"

namespace nox::render
{
	struct GraphicBufferDesc;
	class VertexBuffer;
	class IndexBuffer;

	namespace command
	{
		class CommandBase;
	}
}

namespace nox::render::os
{
	class DeviceDX12 : public nox::render::RenderDevice
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

	private:
		::ID3D12Device14* device_;
		::IDXGISwapChain4* swap_chain_;
		::ID3D12GraphicsCommandList10* command_list_;
		::ID3D12CommandAllocator* command_allocator_;
	};
}