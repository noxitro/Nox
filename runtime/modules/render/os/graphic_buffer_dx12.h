//	Copyright (C) 2026 NOX ENGINE All rights reserved.

///	@file	graphic_buffer_dx12.h
///	@brief	graphic_buffer_dx12
#pragma once
#include	"os_dx12.h"
#include	"../graphic_buffer.h"

namespace nox::render::os
{
	class VertexBufferDX12 final : public nox::render::VertexBuffer
	{
		NOX_DECLARE_OBJECT(nox::render::os::VertexBufferDX12, nox::render::VertexBuffer);
	public:
		inline constexpr explicit VertexBufferDX12(::ID3D12Resource2* resource, nox::uint32 byte_size, nox::uint32 stride_size)noexcept :
			nox::render::VertexBuffer(byte_size, stride_size),
			resource_(resource),
			view_{}
		{
			if (resource_ != nullptr)
			{
				view_.BufferLocation = resource_->GetGPUVirtualAddress();
				view_.SizeInBytes = byte_size;
				view_.StrideInBytes = stride_size;
			}
		}
		inline constexpr ~VertexBufferDX12()noexcept override
		{
			if (resource_ != nullptr)
			{
				resource_->Release();
				resource_ = nullptr;
			}
		}
		inline constexpr const ::D3D12_VERTEX_BUFFER_VIEW& GetView()const noexcept { return view_; }

	private:
		::ID3D12Resource2* resource_;
		::D3D12_VERTEX_BUFFER_VIEW view_;
	};
}