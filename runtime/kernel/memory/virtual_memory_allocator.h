//	Copyright (C) 2026 NOX ENGINE All rights reserved.

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