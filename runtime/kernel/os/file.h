//	Copyright (C) 2026 NOX ENGINE All rights reserved.

///	@file	file.h
///	@brief	file
#pragma once
#include	"../basic_definition.h"

#if NOX_WIN64
#include	"detail/file_win64.h"

namespace nox::os
{
	using File = nox::os::detail::FileWin64;
}
#endif // NOX_WIN64