//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	atomic.h
///	@brief	atomic
#pragma once
#include	"../basic_definition.h"

#if NOX_WIN64
#include	"detail/atomic_win64.h"
#else
static_assert(false);
#endif // NOX_WIN64

namespace nox
{
	template<class T>
	using Atomic = std::atomic<T>;
}