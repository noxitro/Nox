///	@file	os.h
///	@brief	os
#pragma once

#if NOX_X64
#include	"mutex_x64.h"

namespace nox::os
{
	using Mutex = os::detail::MutexX64;
}

#endif // 0


