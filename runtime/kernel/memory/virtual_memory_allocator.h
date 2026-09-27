//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	virtual_memory_allocator.h
///	@brief	virtual_memory_allocator
#pragma once

#include	"../basic_definition.h"
#include	"memory_definition.h"

namespace nox::memory::virtual_memory_allocator
{
	void* Allocate(std::size_t size, std::size_t alignment);
	void Free(void* ptr);
}