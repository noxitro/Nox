//	Copyright (C) 2024 NOX ENGINE All Rights Rserved.

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