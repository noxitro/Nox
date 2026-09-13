//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	thread.h
///	@brief	thread
#pragma once

#include	"../basic_definition.h"

#if NOX_WIN64
#include	"detail/thread_win64.h"
namespace nox::os
{
	using Thread = nox::os::detail::ThreadWin64;
}
#else
static_assert(false);
#endif // NOX_WIN64
