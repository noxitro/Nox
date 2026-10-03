//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	file.h
///	@brief	file
#pragma once
#include	"../basic_definition.h"

#if NOX_WIN64
#include	"detail/file_win64.h"

namespace nox::io
{
	using File = nox::detail::FileWin64;
	using ReadOnlyMappedFile = nox::detail::ReadOnlyMappedFileWin64;
}
#endif // NOX_WIN64