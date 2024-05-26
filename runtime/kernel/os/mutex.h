///	@file	mutex.h
///	@brief	mutex
#pragma once
#include	"../basic_definition.h"

#if NOX_WIN64
#include	"detail/mutex_win64.h"
namespace nox::os
{
	using Mutex = os::detail::MutexWin64;
}
#else
	static_assert(false);
#endif // NOX_WIN64
