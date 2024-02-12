#include	"stdafx.h"
#include	"memory.h"

#include	"../if/algorithm.h"

using namespace nox;
using namespace nox::memory;

namespace
{
//	std::array<int32, util::ToUnderlying(AreaType::_Max)> 
}

not_null<void*>	memory::Allocate(const size_t size, const AreaType areaType)
{
	return std::malloc(size);
}

void	memory::Deallocate(not_null<void*> ptr)
{
	std::free(ptr.get());
}