// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

#include	"pch.h"
#include	"stl_allocate_adapter.h"

#include	"nox_memory.h"
#include	"assertion.h"

void* nox::memory::detail::AllocateStlAllocateAdapter(size_t size)
{
	return nox::memory::Allocate(size, nox::memory::InstanceType::Stl);
}

void	nox::memory::detail::DeallocateStlAllocateAdapter(void* ptr, size_t num)
{
	//NOX_ASSERT(num == 1, u"複数解放には対応していません");
	nox::memory::Deallocate(ptr);
}
