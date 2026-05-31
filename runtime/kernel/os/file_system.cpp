//	Copyright (C) 2024 NOX ENGINE All Rights Rserved.

///	@file	file_system.cpp
///	@brief	file_system
#include	"pch.h"
#include	"file_system.h"

#include	"../assertion.h"
#include	"../unicode_converter.h"
#include	"os_definition.h"

#if NOX_WINDOWS
#include	"windows.h"
#endif // NOX_WINDOWS

nox::U16String	nox::filesystem::GetCurrentPath()
{
#if NOX_WINDOWS
	std::array<nox::wchar16, nox::os::k_max_path_length> path_buffer;
	if (::GetCurrentDirectoryW(static_cast<::DWORD>(path_buffer.size()), path_buffer.data()) == 0)
	{
		NOX_ASSERT(false, u"");
	}

	return nox::U16String(nox::unicode::ConvertString<nox::StlU16String>(path_buffer.data()));
#else
	static_assert(false);
#endif
}