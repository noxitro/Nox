//	Copyright (C) 2024 NOX ENGINE All Rights Rserved.

///	@file	debug_break.cpp
///	@brief	debug_break
#include	"stdafx.h"
#include	"debug_break.h"

#include	"basic_definition.h"

void ::nox::DebugBreak()
{
#if! NOX_MASTER
#if defined(_MSC_VER)
	::__debugbreak();
#endif // defined(_MSC_VER)
#endif // NOX_DEBUG
}