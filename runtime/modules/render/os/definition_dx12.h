//	Copyright (C) 2026 NOX ENGINE All rights reserved.

///	@file	definition_dx12.h
///	@brief	definition_dx12
#pragma once
#include	"os_dx12.h"

namespace nox::render::os
{
	inline constexpr ::D3D12_HEAP_TYPE ToHeapType(nox::render::HeapType v)noexcept
	{
		switch (v)
		{
		case nox::render::HeapType::Default:	return D3D12_HEAP_TYPE_DEFAULT;
		case nox::render::HeapType::Upload:	return D3D12_HEAP_TYPE_UPLOAD;
		case nox::render::HeapType::Readback:	return D3D12_HEAP_TYPE_READBACK;
		case nox::render::HeapType::Custom:	return D3D12_HEAP_TYPE_CUSTOM;
		case nox::render::HeapType::GpuUpload:	return D3D12_HEAP_TYPE_UPLOAD;
		default:
			NOX_ASSERT(false, u"Invalid HeapType");
			return D3D12_HEAP_TYPE_DEFAULT;
		}
	}
}