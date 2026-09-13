//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	graphic_buffer.h
///	@brief	graphic_buffer
#pragma once
#include	"render_resource.h"
#include	"definition.h"

namespace nox::render
{
	struct GraphicBufferDesc
	{
		nox::uint32 byte_size = 0;
		nox::render::UsageType usage_type = nox::render::UsageType::Default;
		nox::render::BindFlag bind_flag = nox::render::BindFlag::Unknown;
		nox::render::CpuAccessFlag cpu_access_flag = nox::render::CpuAccessFlag::None;
		nox::render::ResourceMiscFlag misc_flag = nox::render::ResourceMiscFlag::None;
	};

	struct GraphicBufferSubResourceDesc
	{
		const void* system_memory = nullptr;
		nox::uint32 system_memory_pitch = 0;
		nox::uint32 system_memory_slice_pitch = 0;
	};

	class GraphicBuffer : public nox::render::RenderResource
	{
		NOX_DECLARE_OBJECT(nox::render::GraphicBuffer, nox::render::RenderResource);
	protected:
		inline constexpr explicit GraphicBuffer(nox::uint32 byte_size, nox::render::BindFlag bind_flag)noexcept :
			byte_size_(byte_size),
			bind_flag_(bind_flag)
		{
		}

		inline constexpr ~GraphicBuffer()noexcept override {  }
	public:
		inline constexpr bool IsBindFlag(BindFlag flag)const noexcept { return nox::util::IsBitAnd(bind_flag_, flag); }
		inline constexpr nox::uint32 GetByteSize()const noexcept { return byte_size_; }
		inline constexpr BindFlag GetBindFlag()const noexcept { return bind_flag_; }

	private:
		const nox::uint32 byte_size_;
		const nox::render::BindFlag bind_flag_;
	};

	class VertexBuffer : public nox::render::GraphicBuffer
	{
		NOX_DECLARE_OBJECT(nox::render::VertexBuffer, nox::render::GraphicBuffer);
	public:
		inline constexpr explicit VertexBuffer(nox::uint32 byte_size, nox::uint32 stride_size)noexcept :
			GraphicBuffer(byte_size, nox::render::BindFlag::VertexBuffer),
			stride_size_(stride_size)
		{
		}
		inline constexpr ~VertexBuffer()noexcept override {}
		inline constexpr nox::uint32 GetStrideSize()const noexcept { return stride_size_; }

	private:
		const nox::uint32 stride_size_;
	};

	class IndexBuffer : public nox::render::GraphicBuffer
	{
		NOX_DECLARE_OBJECT(nox::render::IndexBuffer, nox::render::GraphicBuffer);
	public:
		inline constexpr explicit IndexBuffer(nox::uint32 byte_size, nox::render::FormatType element_format)noexcept :
			GraphicBuffer(byte_size, nox::render::BindFlag::IndexBuffer),
			element_format_(element_format)
		{
		}
		inline constexpr ~IndexBuffer()noexcept override {}
		inline constexpr nox::render::FormatType GetElementFormat()const noexcept { return element_format_; }

	private:
		nox::render::FormatType element_format_;
	};
}