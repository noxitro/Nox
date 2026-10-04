//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	debug_break.cpp
///	@brief	debug_break
#include	"pch.h"
#include	"debug_break.h"

#include	"basic_definition.h"

void ::nox::DebugBreak()
{
#if !NOX_MASTER
#if NOX_COMPILER_MSVC
	::__debugbreak();
#endif // NOX_COMPILER_MSVC
#endif // !NOX_MASTER
}