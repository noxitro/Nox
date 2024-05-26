#include	"stdafx.h"
#include	"memory.h"

#include	"../algorithm.h"

#include	"../os/thread.h"
using namespace nox;
using namespace nox::memory;

namespace
{
}

void*	memory::Allocate(const size_t size, const AreaType area_type)
{
	return std::malloc(size);
}

void* memory::Allocate(const size_t size, size_t alignment, const AreaType area_type)
{
	return ::_aligned_malloc(size, alignment);
}

void	memory::Deallocate(not_null<void*> ptr)
{
	std::free(ptr.get());
}

void	memory::Deallocate(not_null<void*> ptr, size_t alignment)
{
	::_aligned_free(ptr.get());
}