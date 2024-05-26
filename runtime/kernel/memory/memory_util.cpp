///	@file	memory_util.cpp
///	@brief	memory_util
#include	"stdafx.h"
#include	"memory_util.h"

#if NOX_WINDOWS
#include	"../os/windows.h"
#endif // NOX_WIN64


#include	<memory>


using namespace nox;

void	memory::detail::ZeroMemImpl(void* ptr, size_t size)
{
#if NOX_WINDOWS
	::SecureZeroMemory(ptr, size);
#else
	std::memset(ptr, 0, size);
#endif // NOX_WIN64
}

not_null<void*>	memory::Copy(not_null<void*> dest, not_null<const void*> src, size_t size)
{
	return std::memcpy(dest.get(), src.get(), size);
}

nox::int32	memory::Copy(not_null<void*> dest, size_t dest_size, not_null<const void*> src, size_t source_size)
{
	return ::memcpy_s(dest.get(), dest_size, src.get(), source_size);
}

nox::int32	memory::WideCopy(not_null<wchar16*> dest, size_t dest_size, not_null<const wchar16*> source, size_t source_size)
{
	return ::wmemcpy_s(dest.get(), dest_size, source.get(), source_size);
}