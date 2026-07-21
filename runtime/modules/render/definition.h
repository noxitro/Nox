//	Copyright (C) 2026 NOX ENGINE All rights reserved.

///	@file	definition.h
///	@brief	definition
#pragma once

#define NOX_RENDER_DX12 1
#define NOX_RENDER_DX11 0
#define NOX_RENDER_VULKAN    0

namespace nox::render
{
	enum class GraphicsApi : nox::uint8
	{
		DirectX12,
		Vulkan,
		WebGPU,
		_Max
	};

	enum class BindFlag : nox::uint16
	{
		Unknown = 0,
		VertexBuffer = 1 << 0,
		IndexBuffer = 1 << 1,
		ConstantBuffer = 1 << 2,
		ShaderResource = 1 << 3,
		StreamOutput = 1 << 4,
		RenderTarget = 1 << 5,
		DepthStencil = 1 << 6,
		UnorderedAccess = 1 << 7,
		Decoder = 1 << 9,
		VideoEncoder = 1 << 10,
	};

	enum class UsageType : nox::uint8
	{
		Default,
		Immutable,
		Dynamic,
		Staging,
	};

	enum class HeapType : nox::uint8
	{
		Default,
		Upload,
		Readback,
		Custom,
		GpuUpload,
		_Max,
	};

	enum class ShaderStage : nox::uint8
	{
		VS,
		HS,
		DS,
		GS,
		PS,
		CS,
		_Max
	};

	enum class ShaderInputType : nox::uint8 
	{
		ConstantBuffer,
		TextureBuffer,
		Texture,
		Sampler,
	};

	enum class CpuAccessFlag : nox::uint8
	{
		None,
		Read = 1 << 0,
		Write = 1 << 1,
		ReadWrite = Read | Write,
	};

	enum class ResourceMiscFlag : nox::uint8
	{
		None,
		GenerateMips = 1 << 0,
		Shared = 1 << 1,
	};

	enum class FormatType : nox::uint8
	{
		Unknown,
		R8G8B8A8_UNorm,
		R16G16B16A16_Float,
		R32G32B32A32_Float,
		D24_UNorm_S8_UInt,
		D32_Float,
	};

	enum class IndexBufferFormatType : nox::uint8
	{
		Index16,
		Index32,
	};

	enum class SemanticType : nox::uint8
	{
		Position,
		Normal,
		Tangent,
		Bitangent,
		Color,
		TexCoord,
		_Max
	};
}