#include	"stdafx.h"
#include	"stl_allocate_adapter.h"

#include	"nox_memory.h"

void* nox::memory::detail::AllocateStlAllocateAdapter(size_t size)
{
	return nox::memory::Allocate(size, nox::memory::InstanceType::Stl);
}

void	nox::memory::detail::DeallocateStlAllocateAdapter(void* ptr)
{
	nox::memory::Deallocate(ptr);
}
