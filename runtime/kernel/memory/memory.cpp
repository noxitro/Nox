#include	"stdafx.h"
#include	"memory.h"

#include	"../algorithm.h"

#include	"../os/thread.h"

namespace
{
}

void* nox::memory::Allocate(const size_t size, const AreaType area_type)
{
	return std::malloc(size);
}

void* nox::memory::Allocate(const size_t size, size_t alignment, const AreaType area_type)
{
	return nullptr;
	//return ::_aligned_malloc(size, alignment);
}

void	nox::memory::Deallocate(nox::not_null<void*> ptr)
{
	std::free(ptr.get());
}

void	nox::memory::Deallocate(nox::not_null<void*> ptr, size_t alignment)
{
	::_aligned_free(ptr.get());
}