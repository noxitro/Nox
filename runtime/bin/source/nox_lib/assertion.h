// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

///	@file	assertion.h
///	@brief	assertion
#pragma once
#include	<string_view>
#include	<source_location>
#include	<cassert>
#include	<stacktrace>

namespace nox
{
	void Assert(std::u16string_view message, std::wstring_view filename, const std::source_location location)noexcept
	{
		::_wassert(reinterpret_cast<const wchar_t*>(message.data()), filename.data(), location.line());
	}
}

#define NOX_ASSERT(condition, message) \
	((void)(			\
	(!!(condition)) || \
	(::nox::Assert(message, __FILEW__, ::std::source_location::current()),0))\
	)