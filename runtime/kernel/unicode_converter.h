//	Copyright (C) 2024 NOX ENGINE All Rights Rserved.

///	@file	unicode_converter.h
///	@brief	unicode_converter
#pragma once

#include	"convert_string.h"

namespace nox::unicode
{
#pragma region cstring
	nox::StdNString	ConvertNString(std::u8string_view str_view);
	nox::StdNString	ConvertNString(std::u16string_view str_view);
	nox::StdNString	ConvertNString(std::u32string_view str_view);
	nox::StdNString	ConvertNString(std::wstring_view str_view);

	template<std::same_as<char> To, class From>
		requires(std::is_void_v<std::void_t<decltype(::nox::unicode::ConvertNString(std::declval<From>()))>>)
	inline	auto	ConvertString(From&& str, std::span<To> dest_buffer)
	{
		return ::nox::unicode::ConvertNString(str, dest_buffer);
	}

	template<std::same_as<::nox::StdNString> To, class From>
		requires(std::is_void_v<std::void_t<decltype(::nox::unicode::ConvertNString(std::declval<From>()))>>)
	inline	To	ConvertString(From&& str)
	{
		return ::nox::unicode::ConvertNString(str);
	}
#pragma endregion

#pragma region char8

	nox::StdU8String	ConvertU8String(std::string_view str_view);
	nox::StdU8String	ConvertU8String(std::u16string_view str_view);
	nox::StdU8String	ConvertU8String(std::u32string_view str_view);
	inline nox::StdU8String	ConvertU8String(std::wstring_view str_view) 
	{
		return ConvertU8String({ reinterpret_cast<const char16*>(str_view.data()), str_view.size() }); 
	}

	std::u8string_view	ConvertU8String(std::string_view str_view, std::span<nox::char8> dest_buffer);
	std::u8string_view	ConvertU8String(std::u16string_view str_view, std::span<nox::char8> dest_buffer);
	std::u8string_view	ConvertU8String(std::u32string_view str_view, std::span<nox::char8> dest_buffer);
	inline std::u8string_view	ConvertU8String(std::wstring_view str_view, std::span<nox::char8> dest_buffer)
	{
		return ConvertU8String({ reinterpret_cast<const char16*>(str_view.data()), str_view.size() }, dest_buffer);
	}

	template<std::same_as<::nox::StdU8String> To, class From>
		requires(std::is_void_v<std::void_t<decltype(unicode::ConvertU8String(std::declval<From>()))>>)
	inline	To	ConvertString(From&& str)
	{
		return ::nox::unicode::ConvertU8String(str);
	}
	template<std::same_as<char8> To, class From>
		requires(std::is_void_v<std::void_t<decltype(::nox::unicode::ConvertU8String(std::declval<From>()))>>)
	inline	auto	ConvertString(From&& str, std::span<To> dest_buffer)
	{
		return ::nox::unicode::ConvertU8String(str, dest_buffer);
	}
#pragma endregion

#pragma region char16
	nox::StdU16String	ConvertU16String(std::string_view str_view);
	nox::StdU16String	ConvertU16String(std::u8string_view str_view);
	nox::StdU16String	ConvertU16String(std::u32string_view str_view);
	inline nox::StdU16String	ConvertU16String(std::wstring_view str_view)
	{
		return nox::StdU16String(reinterpret_cast<const char16*>(str_view.data()), str_view.size());
	}

	std::u16string_view	ConvertU16String(const std::u8string_view str_view, std::span<char16> dest_buffer);
	std::u16string_view	ConvertU16String(const std::u32string_view str_view, std::span<char16> dest_buffer);
	std::u16string_view	ConvertU16String(const std::wstring_view str_view, std::span<char16> dest_buffer);
	inline std::u16string_view	ConvertU16String(const std::string_view str_view, std::span<char16> dest_buffer)
	{
		return ConvertU16String({ reinterpret_cast<const char8*>(str_view.data()), str_view.size() }, dest_buffer);
	}

	template<std::same_as<nox::StdU16String> To, class From>
		requires(std::is_void_v<std::void_t<decltype(::nox::unicode::ConvertU16String(std::declval<From>()))>>)
	inline	To	ConvertString(From&& str)
	{
		return ::nox::unicode::ConvertU16String(str);
	}
	template<std::same_as<::nox::char16> To, class From>
		requires(std::is_void_v<std::void_t<decltype(::nox::unicode::ConvertU16String(std::declval<From>()))>>)
	inline	auto	ConvertString(From&& str, std::span<To> dest_buffer)
	{
		return ::nox::unicode::ConvertU16String(str, dest_buffer);
	}
#pragma endregion

#pragma region wstring
	nox::StdWString	ConvertWString(std::u8string_view str_view);
	nox::StdWString	ConvertWString(std::u32string_view str_view);
	inline nox::StdWString	ConvertWString(std::string_view str_view)
	{
		return ConvertWString({ reinterpret_cast<const char8*>(str_view.data()), str_view.size() });
	}
	inline nox::StdWString	ConvertWString(std::u16string_view str_view)
	{
		return nox::StdWString(util::CharCast<wchar16>(str_view.data()), str_view.length());
	}

	inline void	ConvertWString(const std::u8string_view str_view, std::span<wchar_t> dest_buffer)
	{
		std::span<char16> buffer(util::CharCast<char16>(dest_buffer.data()), dest_buffer.size());
		ConvertU16String(str_view, buffer);
	}

	inline void	ConvertWString(const std::string_view str_view, std::span<wchar_t> dest_buffer)
	{
		ConvertWString({ reinterpret_cast<const char8*>(str_view.data()), str_view.size() }, dest_buffer);
	}

	inline	void	ConvertWString(const std::u32string_view str_view, std::span<wchar_t> dest_buffer)
	{
		std::span<char16> buffer(util::CharCast<char16>(dest_buffer.data()), dest_buffer.size());
		ConvertU16String(str_view, buffer);
	}

	template<std::same_as<::nox::StdWString> To, class From>
		requires(std::is_void_v<std::void_t<decltype(::nox::unicode::ConvertWString(std::declval<From>()))>>)
	inline	To	ConvertString(From&& str)
	{
		return ::nox::unicode::ConvertWString(str);
	}

	template<std::same_as<wchar16> To, class From>
	//	requires(std::is_void_v<std::void_t<decltype(unicode::ConvertWString(std::declval<From>()))>>)
	inline	void	ConvertString(From&& str, std::span<To> dest_buffer)
	{
		::nox::unicode::ConvertWString(str, dest_buffer);
	}
#pragma endregion

#pragma region char32
	::nox::StdU32String	ConvertU32String(std::u8string_view str_view);
	::nox::StdU32String	ConvertU32String(std::u16string_view str_view);
	inline ::nox::StdU32String	ConvertU32String(std::string_view str_view)
	{
		return ConvertU32String({ reinterpret_cast<const char8*>(str_view.data()), str_view.size() });
	}
	inline ::nox::StdU32String	ConvertU32String(std::wstring_view str_view)
	{
		return ConvertU32String({ reinterpret_cast<const char16*>(str_view.data()), str_view.size() });
	}
	
	std::u32string_view	ConvertU32String(const std::u8string_view str_view, std::span<char32> dest_buffer);
	std::u32string_view	ConvertU32String(const std::u16string_view str_view, std::span<char32> dest_buffer);
	inline std::u32string_view	ConvertU32String(const std::string_view str_view, std::span<char32> dest_buffer)
	{
		return ConvertU32String({ reinterpret_cast<const char8*>(str_view.data()), str_view.size() }, dest_buffer);
	}
	inline std::u32string_view	ConvertU32String(const std::wstring_view str_view, std::span<char32> dest_buffer)
	{
		return ConvertU32String({ reinterpret_cast<const char16*>(str_view.data()), str_view.size() }, dest_buffer);
	}

	template<std::same_as<::nox::StdU32String> To, class From>
		requires(std::is_void_v<std::void_t<decltype(unicode::ConvertU32String(std::declval<From>()))>>)
	inline	To	ConvertString(From&& str)
	{
		return ::nox::unicode::ConvertU32String(str);
	}
	template<std::same_as<::nox::char32> To, class From>
		requires(std::is_void_v<std::void_t<decltype(unicode::ConvertU32String(std::declval<From>()))>>)
	inline	auto	ConvertString(From&& str, std::span<To> dest_buffer)
	{
		return ::nox::unicode::ConvertU32String(str, dest_buffer);
	}
#pragma endregion
}