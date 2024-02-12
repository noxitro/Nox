//	Copyright (C) 2024 NOX ENGINE All Rights Rserved.

///	@file	thread.h
///	@brief	thread
#pragma once

#include	"../if/basic_definition.h"

#if NOX_WIN64
#include	"detail/mutex_x64.h"
namespace nox::os
{
	//using Mutex = os::detail::MutexWin64;
}
#else
static_assert(false);
#endif // NOX_WIN64
