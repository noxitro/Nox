//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	assertion_kernel.cpp
///	@brief	assertion_kernel
#include	"pch.h"
#include	"assertion_kernel.h"

#include	"assertion.h"

void nox::assertion::detail::AssertKernel(std::u16string_view error_category, std::u8string_view message, std::wstring_view filename, const std::source_location location)
{
	nox::assertion::detail::Assert(error_category, message, filename, location);
}

void nox::assertion::detail::AssertKernel(std::u16string_view error_category, std::u16string_view message, std::wstring_view filename, const std::source_location location)
{
	nox::assertion::detail::Assert(error_category, message, filename, location);
}