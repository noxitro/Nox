///	@file	memory_util.cpp
///	@brief	memory_util
#include	"stdafx.h"
#include	"memory_util.h"

#if NOX_X64
#include	"../os/x64.h"
#endif // NOX_X64


#include	<memory>


using namespace nox;

void	memory::detail::ZeroMemImpl(void* ptr, size_t size)
{
#if NOX_X64
	::SecureZeroMemory(ptr, size);
#else
	std::memset(ptr, 0, size);
#endif // NOX_X64
}