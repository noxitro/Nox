//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	stl_string.h
///	@brief	stl_string
#pragma once
#include	<string>
#include	"memory/stl_allocate_adapter.h"
#include	"basic_type.h"
namespace nox
{
	template<class ValueType>
	using StlBasicString = std::basic_string< ValueType, std::char_traits<ValueType>, nox::memory::StlAllocateAdapter<ValueType>>;

	using StlCString = StlBasicString<char>;
	using StlNString = StlBasicString<char>;
	using StlWString = StlBasicString<wchar_t>;
	using StlU8String = StlBasicString<nox::char8>;
	using StlU16String = StlBasicString<nox::char16>;
	using StlU32String = StlBasicString<nox::char32>;
}