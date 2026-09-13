// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

///	@file	device_dx12_internal.h
///	@brief	device_dx12_internal
#pragma once
#include	"os_dx12.h"

#if NOX_RENDER_DX12
#include	"../graphic_buffer.h"

namespace nox::render::os
{
	namespace
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
}
#endif