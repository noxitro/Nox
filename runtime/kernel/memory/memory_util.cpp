///	@file	memory_util.cpp
///	@brief	memory_util
#include	"stdafx.h"
#include	"memory_util.h"

#if NOX_WIN64
#include	"../os/x64.h"
#endif // NOX_WIN64


#include	<memory>


using namespace nox;

void	memory::detail::ZeroMemImpl(void* ptr, size_t size)
{
#if NOX_WIN64
	::SecureZeroMemory(ptr, size);
#else
	std::memset(ptr, 0, size);
#endif // NOX_WIN64
}

not_null<void*>	memory::Copy(not_null<void*> dest, not_null<const void*> src, size_t size)
{
	return std::memcpy(dest.get(), src.get(), size);
}