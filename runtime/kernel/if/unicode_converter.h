//	Copyright (C) 2024 NOX ENGINE All Rights Rserved.

///	@file	unicode_converter.h
///	@brief	unicode_converter
#pragma once

#include	"convert_string.h"

namespace nox::unicode
{
#pragma region wstring

	bool	ConvertWString(std::span<wchar_t> buffer, const c8* from, size_t length);

	inline	bool	ConvertWString(std::span<wchar_t> buffer, const c8* from)
	{
		return ConvertWString(buffer, from, util::GetStrLength(from));
	}

	nox::WString	ConvertWString(const char* from, size_t length);
	nox::WString	ConvertWString(const char8* from, size_t length);
	nox::WString	ConvertWString(const char16* from, size_t length);
	nox::WString	ConvertWString(const char32* from, size_t length);

	template<class T>
	inline nox::WString	ConvertWString(const T* from)
	{
		return ConvertWString(from, util::GetStrLength(from));
	}

	template<class T> requires(
		std::is_invocable_v<decltype(static_cast<nox::WString(*)(const T*, size_t)>(nox::unicode::ConvertWString)), const T*, size_t>
		)
		inline nox::WString	ConvertWString(std::basic_string_view<T> str)
	{
		return ConvertWString(str.data(), str.size());
	}

	template<class T> requires(
		std::is_invocable_v<decltype(static_cast<nox::WString(*)(const T*, size_t)>(nox::unicode::ConvertWString)), const T*, size_t>
		)
		inline nox::WString	ConvertWString(std::basic_string<T> str)
	{
		return ConvertWString(str.c_str(), str.size());
	}
#pragma endregion

	//template<class To, class From>
	//To	ConvertString(const From* str);
	//template<class To, class From>
	//To	ConvertString(const std::basic_string_view<From> str);
	//template<class To, class From>
	//To	ConvertString(const std::basic_string<From>& str);

	template<std::same_as<nox::WString> To, class From> requires(nox::IsCharTypeValue<From>)
		inline	To	ConvertString(const From* str)
	{
		return unicode::ConvertWString(str);
	}
	template<std::same_as<nox::WString> To, class From> requires(nox::IsCharTypeValue<From>)
		inline	To	ConvertString(const std::basic_string_view<From> str)
	{
		return unicode::ConvertWString(str);
	}
	template<std::same_as<nox::WString> To, class From> requires(nox::IsCharTypeValue<From>)
		inline	To	ConvertString(const nox::BasicString<From>& str)
	{
		return unicode::ConvertWString(str);
	}
}