//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	cpu_relax.cpp
///	@brief	cpu_relax
#include	"pch.h"
#include	"cpu_relax.h"

#if defined(_M_X64) || defined(__x86_64__)
#include <immintrin.h>
#elif defined(_M_ARM64)
#include <intrin.h>
#endif

void nox::CpuRelax()noexcept
{
#if defined(_M_X64) || defined(__x86_64__)
	::_mm_pause();
#elif defined(_M_ARM64)
	::__yield();
#elif defined(__aarch64__)
	__asm__ __volatile__("yield");
#endif
}