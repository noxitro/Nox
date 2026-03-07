//	Copyright (C) 2024 NOX ENGINE All Rights Rserved.

///	@file	nox_string_view.cpp
///	@brief	nox_string_view
#include	"stdafx.h"
#include	"nox_string_view.h"
#include	"unicode_converter.h"

template<>
std::u8string_view nox::BasicStringView<char>::ToUTF8Impl(std::span<nox::char8> buffer) const
{
	return nox::unicode::ConvertU8String(view_, buffer);
}

template<>
std::u8string_view nox::BasicStringView<nox::wchar16>::ToUTF8Impl(std::span<nox::char8> buffer) const
{
	return nox::unicode::ConvertU8String(view_, buffer);
}

template<>
std::u8string_view nox::BasicStringView<nox::char16>::ToUTF8Impl(std::span<nox::char8> buffer) const
{
	return nox::unicode::ConvertU8String(view_, buffer);
}

template<>
std::u8string_view nox::BasicStringView<nox::char32>::ToUTF8Impl(std::span<nox::char8> buffer) const
{
	return nox::unicode::ConvertU8String(view_, buffer);
}