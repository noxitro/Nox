//	Copyright (C) 2024 NOX ENGINE All Rights Rserved.

///	@file	unicode_converter.h
///	@brief	unicode_converter
#pragma once

#include	"convert_string.h"

namespace nox::unicode
{
#pragma region cstring
	nox::StlNString	ConvertNString(std::u8string_view str_view);
	nox::StlNString	ConvertNString(std::u16string_view str_view);
	nox::StlNString	ConvertNString(std::u32string_view str_view);
	nox::StlNString	ConvertNString(std::wstring_view str_view);

	std::string_view	ConvertNString(std::u8string_view str_view, std::span<char> dest_buffer);
	std::string_view	ConvertNString(std::u16string_view str_view, std::span<char> dest_buffer);
	std::string_view	ConvertNString(std::u32string_view str_view, std::span<char> dest_buffer);
	std::string_view	ConvertNString(std::wstring_view str_view, std::span<char> dest_buffer);

	template<std::same_as<char> To, class From>
		requires(std::is_void_v<std::void_t<decltype(::nox::unicode::ConvertNString(std::declval<From>()))>>)
	inline	auto	ConvertString(From&& str, std::span<To> dest_buffer)
	{
		return ::nox::unicode::ConvertNString(str, dest_buffer);
	}

	template<std::same_as<::nox::StlNString> To, class From>
		requires(std::is_void_v<std::void_t<decltype(::nox::unicode::ConvertNString(std::declval<From>()))>>)
	inline	To	ConvertString(From&& str)
	{
		return ::nox::unicode::ConvertNString(str);
	}
#pragma endregion

#pragma region char8

	nox::StlU8String	ConvertU8String(std::string_view str_view);
	nox::StlU8String	ConvertU8String(std::u16string_view str_view);
	nox::StlU8String	ConvertU8String(std::u32string_view str_view);
	inline nox::StlU8String	ConvertU8String(std::wstring_view str_view) 
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

	template<std::same_as<::nox::StlU8String> To, class From>
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
	nox::StlU16String	ConvertU16String(std::string_view str_view);
	nox::StlU16String	ConvertU16String(std::u8string_view str_view);
	nox::StlU16String	ConvertU16String(std::u32string_view str_view);
	inline nox::StlU16String	ConvertU16String(std::wstring_view str_view)
	{
		return nox::StlU16String(reinterpret_cast<const char16*>(str_view.data()), str_view.size());
	}

	std::u16string_view	ConvertU16String(const std::u8string_view str_view, std::span<char16> dest_buffer);
	std::u16string_view	ConvertU16String(const std::u32string_view str_view, std::span<char16> dest_buffer);
	std::u16string_view	ConvertU16String(const std::wstring_view str_view, std::span<char16> dest_buffer);
	inline std::u16string_view	ConvertU16String(const std::string_view str_view, std::span<char16> dest_buffer)
	{
		return ConvertU16String({ reinterpret_cast<const char8*>(str_view.data()), str_view.size() }, dest_buffer);
	}

	template<std::same_as<nox::StlU16String> To, class From>
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
	nox::StlWString	ConvertWString(std::u8string_view str_view);
	nox::StlWString	ConvertWString(std::u32string_view str_view);
	inline nox::StlWString	ConvertWString(std::string_view str_view)
	{
		return ConvertWString({ reinterpret_cast<const char8*>(str_view.data()), str_view.size() });
	}
	inline nox::StlWString	ConvertWString(std::u16string_view str_view)
	{
		return nox::StlWString(util::CharCast<wchar16>(str_view.data()), str_view.length());
	}

	inline std::wstring_view	ConvertWString(const std::u8string_view str_view, std::span<wchar_t> dest_buffer)
	{
		std::span<char16> buffer(util::CharCast<char16>(dest_buffer.data()), dest_buffer.size());
		auto result = ConvertU16String(str_view, buffer);
		return std::wstring_view(util::CharCast<wchar16>(result.data()), result.size());
	}

	inline std::wstring_view	ConvertWString(const std::string_view str_view, std::span<wchar_t> dest_buffer)
	{
		return ConvertWString({ reinterpret_cast<const char8*>(str_view.data()), str_view.size() }, dest_buffer);
	}

	inline std::wstring_view	ConvertWString(const std::u32string_view str_view, std::span<wchar_t> dest_buffer)
	{
		std::span<char16> buffer(util::CharCast<char16>(dest_buffer.data()), dest_buffer.size());
		auto result = ConvertU16String(str_view, buffer);
		return std::wstring_view(util::CharCast<wchar16>(result.data()), result.size());
	}

	template<std::same_as<::nox::StlWString> To, class From>
		requires(std::is_void_v<std::void_t<decltype(::nox::unicode::ConvertWString(std::declval<From>()))>>)
	inline	To	ConvertString(From&& str)
	{
		return ::nox::unicode::ConvertWString(str);
	}

	template<std::same_as<wchar16> To, class From>
	//	requires(std::is_void_v<std::void_t<decltype(unicode::ConvertWString(std::declval<From>()))>>)
	inline	auto	ConvertString(From&& str, std::span<To> dest_buffer)
	{
		return ::nox::unicode::ConvertWString(str, dest_buffer);
	}
#pragma endregion

#pragma region char32
	::nox::StlU32String	ConvertU32String(std::u8string_view str_view);
	::nox::StlU32String	ConvertU32String(std::u16string_view str_view);
	inline ::nox::StlU32String	ConvertU32String(std::string_view str_view)
	{
		return ConvertU32String({ reinterpret_cast<const char8*>(str_view.data()), str_view.size() });
	}
	inline ::nox::StlU32String	ConvertU32String(std::wstring_view str_view)
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

	template<std::same_as<::nox::StlU32String> To, class From>
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