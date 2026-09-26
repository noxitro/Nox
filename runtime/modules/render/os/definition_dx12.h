//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	definition_dx12.h
///	@brief	definition_dx12
#pragma once
#include	"os_dx12.h"

namespace nox::render::os
{
	namespace
	{
		template<class Pair, size_t N>
		consteval bool IsDuplicateKey(const std::array<Pair, N>& arr) noexcept
		{
			for (size_t i = 0; i < N; ++i)
			{
				for (size_t j = i + 1; j < N; ++j)
				{
					if (arr[i].first == arr[j].first)
					{
						return true;
					}
				}
			}
			return false;
		}
	}

	inline constexpr ::D3D12_HEAP_TYPE ToHeapType(nox::render::HeapType v)noexcept
	{
		switch (v)
		{
		case nox::render::HeapType::Default:	return ::D3D12_HEAP_TYPE_DEFAULT;
		case nox::render::HeapType::Upload:	return ::D3D12_HEAP_TYPE_UPLOAD;
		case nox::render::HeapType::Readback:	return ::D3D12_HEAP_TYPE_READBACK;
		case nox::render::HeapType::Custom:	return ::D3D12_HEAP_TYPE_CUSTOM;
		case nox::render::HeapType::GpuUpload:	return ::D3D12_HEAP_TYPE_UPLOAD;
		default:
			NOX_ASSERT(false, u"Invalid HeapType");
			return ::D3D12_HEAP_TYPE_DEFAULT;
		}
	}
}