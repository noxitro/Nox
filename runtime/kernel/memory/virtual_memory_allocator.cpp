//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	virtual_memory_allocator.cpp
///	@brief	virtual_memory_allocator
#include	"pch.h"
#include	"virtual_memory_allocator.h"

namespace nox::memory::virtual_memory_allocator 
{
	namespace
	{

	}
}

namespace nox::math
{
	template<std::unsigned_integral T>
	inline constexpr T Log2(T value) noexcept
	{
		// x==0 は未定義にしたくなければ適当に返す（ここでは0）
		if (value == 0)
		{
			return 0;
		}

		T r = 0;
		while (value >>= 1)
		{
			++r;
		}
		return r;
	}

	template<std::unsigned_integral T>
	inline constexpr T Ceil(T value)noexcept
	{
		if (value == 0)
		{
			return 0;
		}
		T r = 1;
		while (r < value)
		{
			r <<= 1;
		}
		return r;
	}
}

void* nox::memory::virtual_memory_allocator::Allocate(std::size_t size, std::size_t alignment)
{
	return nullptr;
}

void nox::memory::virtual_memory_allocator::Free(void* ptr)
{
	constexpr auto nn = std::bit_width(5U);
}