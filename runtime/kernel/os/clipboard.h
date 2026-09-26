//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	clipboard.h
///	@brief	clipboard
#pragma once

#include	"../basic_definition.h"

#if NOX_WIN64
#include	"detail/clipboard_win64.h"
namespace nox::os
{
	using Clipboard = nox::os::detail::ClipboardWin64;
}
#else
static_assert(false);
#endif // NOX_WIN64