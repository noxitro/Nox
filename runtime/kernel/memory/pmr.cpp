//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	pmr.cpp
///	@brief	pmr
#include	"pch.h"
#include	"pmr.h"
#include    "nox_memory.h"

namespace nox::memory::detail
{
    namespace
    {
		constinit nox::memory::detail::PmrMemoryResource pmr_resource;
    }
}

void* nox::memory::detail::PmrMemoryResource::do_allocate(std::size_t bytes, std::size_t alignment)
{
    // エンジン独自のアロケータを呼ぶ
    return nox::memory::Allocate(bytes, alignment, nox::memory::InstanceType::Stl);
}

void nox::memory::detail::PmrMemoryResource::do_deallocate(void* ptr, std::size_t , std::size_t)
{
    nox::memory::Deallocate(ptr);
}

nox::memory::detail::PmrMemoryResource& nox::memory::detail::GetPmrMemoryResource()noexcept
{
    return nox::memory::detail::pmr_resource;
}