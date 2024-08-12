//	Copyright (C) 2024 NOX ENGINE All Rights Rserved.

///	@file	file_system.cpp
///	@brief	file_system
#include	"stdafx.h"
#include	"file_system.h"

#include	"../assertion.h"
#include	"../unicode_converter.h"
#include	"os_definition.h"

#if NOX_WINDOWS
#include	"windows.h"
#endif // NOX_WINDOWS


using namespace nox;
using namespace nox::os;

nox::String	nox::os::file_system::GetCurrentPath()
{
#if NOX_WINDOWS
	std::array<wchar16, os::MAX_PATH_LENGTH> path_buffer;
	if (::GetCurrentDirectoryW(static_cast<::DWORD>(path_buffer.size()), path_buffer.data()) == 0)
	{
		NOX_ASSERT(false, U"");
	}

	return nox::String(unicode::ConvertString<nox::U32String>(path_buffer.data()));
#else
	static_assert(false);
#endif
}