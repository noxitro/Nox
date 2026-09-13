//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	file.h
///	@brief	file
#pragma once
#include	"../basic_definition.h"

#if NOX_WIN64
#include	"detail/file_win64.h"

namespace nox::os
{
	using File = nox::os::detail::FileWin64;
	using ReadOnlyMappedFile = nox::os::detail::ReadOnlyMappedFileWin64;
}
#endif // NOX_WIN64