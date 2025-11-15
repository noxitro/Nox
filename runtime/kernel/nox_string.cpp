//	Copyright (C) 2024 NOX ENGINE All Rights Rserved.

///	@file	string.cpp
///	@brief	string
#include	"stdafx.h"
#include	"nox_string.h"

#include	"unicode_converter.h"
#include	"nox_string_view.h"

nox::String::String(class nox::StringView other)noexcept:
	String(std::u16string_view(other))
{

}

nox::NString nox::String::ToNString()const
{
	return nox::unicode::ConvertNString(string_);
}

nox::WString nox::String::ToWString()const
{
	return nox::unicode::ConvertWString(string_);
}

nox::U32String	nox::String::ToU32String()const
{
	return nox::unicode::ConvertU32String(string_);
}