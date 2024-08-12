//	Copyright (C) 2024 NOX ENGINE All Rights Rserved.

///	@file	unicode_converter.h
///	@brief	unicode_converter
#pragma once

#include	"convert_string.h"
#include	"memory/memory_util.h"

namespace nox::unicode
{
#pragma region cstring
	nox::NString	ConvertNString(std::u8string_view str_view);
	nox::NString	ConvertNString(std::u16string_view str_view);
	nox::NString	ConvertNString(std::wstring_view str_view);
	nox::NString	ConvertNString(std::u32string_view str_view);

#pragma endregion

	nox::U8String	ConvertU8String(std::string_view str_view);
	nox::U8String	ConvertU8String(std::u16string_view str_view);
	inline nox::U8String	ConvertU8String(std::wstring_view str_view)
	{
		return ConvertU8String({ reinterpret_cast<const char16*>(str_view.data()), str_view.size() });
	}
	nox::U8String	ConvertU8String(std::u32string_view str_view);

#pragma region char16

	std::span<char16>	ConvertU16String(const std::u8string_view str_view, std::span<char16> dest_buffer);
	inline std::span<char16>	ConvertU16String(const std::string_view str_view, std::span<char16> dest_buffer)
	{
		return ConvertU16String({ reinterpret_cast<const char8*>(str_view.data()), str_view.size() }, dest_buffer);
	}

	inline std::span<char16>	ConvertU16String(const std::wstring_view str_view, std::span<char16> dest_buffer)
	{
		memory::WideCopy(util::CharCast<wchar16>(dest_buffer.data()), dest_buffer.size(), str_view.data(), str_view.size());
		return dest_buffer;
	}

	std::span<char16>	ConvertU16String(const std::u32string_view str_view, std::span<char16> dest_buffer);

	nox::U16String	ConvertU16String(std::u8string_view str_view);
	inline nox::U16String	ConvertU16String(std::string_view str_view)
	{
		return nox::U16String(reinterpret_cast<const char16*>(str_view.data()), str_view.size());
	}

	inline nox::U16String	ConvertU16String(std::wstring_view str_view)
	{
		return nox::U16String(reinterpret_cast<const char16*>(str_view.data()), str_view.size());
	}
	nox::U16String	ConvertU16String(std::u32string_view str_view);

#pragma endregion

#pragma region wstring

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

	nox::WString	ConvertWString(std::u8string_view str_view);

	inline nox::WString	ConvertWString(std::string_view str_view)
	{
		return ConvertWString({ reinterpret_cast<const char8*>(str_view.data()), str_view.size() });
	}

	inline nox::WString	ConvertWString(std::u16string_view str_view)
	{
		return nox::WString(util::CharCast<wchar16>(str_view.data()), str_view.length());
	}

	nox::WString	ConvertWString(std::u32string_view str_view);
#pragma endregion

#pragma region char32
	
	std::span<char32>	ConvertU32String(const std::u8string_view str_view, std::span<char32> dest_buffer);
	inline std::span<char32>	ConvertU32String(const std::string_view str_view, std::span<char32> dest_buffer)
	{
		return ConvertU32String({ reinterpret_cast<const char8*>(str_view.data()), str_view.size() }, dest_buffer);
	}
	std::span<char32>	ConvertU32String(const std::u16string_view str_view, std::span<char32> dest_buffer);
	inline std::span<char32>	ConvertU32String(const std::wstring_view str_view, std::span<char32> dest_buffer)
	{
		return ConvertU32String({ reinterpret_cast<const char16*>(str_view.data()), str_view.size() }, dest_buffer);
	}

	nox::U32String	ConvertU32String(std::u8string_view str_view);
	inline nox::U32String	ConvertU32String(std::string_view str_view)
	{
		return ConvertU32String({ reinterpret_cast<const char8*>(str_view.data()), str_view.size() });
	}
	nox::U32String	ConvertU32String(std::u16string_view str_view);
	inline nox::U32String	ConvertU32String(std::wstring_view str_view)
	{
		return ConvertU32String({ reinterpret_cast<const char16*>(str_view.data()), str_view.size() });
	}

#pragma endregion

	/// @brief span ver
	/// @tparam From 
	/// @tparam To 
	/// @param str 
	/// @param dest_buffer 
	template<std::same_as<char> To, class From>
		requires(std::is_void_v<std::void_t<decltype(unicode::ConvertNString(std::declval<From>()))>>)
	inline	void	ConvertString(From&& str, std::span<To> dest_buffer)
	{
		unicode::ConvertNString(str, dest_buffer);
	}

	template<std::same_as<nox::NString> To, class From>
		requires(std::is_void_v<std::void_t<decltype(unicode::ConvertNString(std::declval<From>()))>>)
	inline	To	ConvertString(From&& str)
	{
		return unicode::ConvertNString(str);
	}

	template<std::same_as<nox::WString> To, class From>
		requires(std::is_void_v<std::void_t<decltype(unicode::ConvertWString(std::declval<From>()))>>)
		inline	To	ConvertString(From&& str)
	{
		return unicode::ConvertWString(str);
	}


		/// @brief 
		/// @tparam From 
		/// @tparam To 
		/// @param str 
		/// @param dest_buffer 
		/// @return 
		template<std::same_as<wchar16> To, class From>
		//	requires(std::is_void_v<std::void_t<decltype(unicode::ConvertWString(std::declval<From>()))>>)
		inline	void	ConvertString(From&& str, std::span<To> dest_buffer)
		{
			unicode::ConvertWString(str, dest_buffer);
		}

	template<std::same_as<nox::U8String> To, class From>
		requires(std::is_void_v<std::void_t<decltype(unicode::ConvertU8String(std::declval<From>()))>>)
	inline	To	ConvertString(From&& str)
	{
		return unicode::ConvertU8String(str);
	}

	template<std::same_as<char8> To, class From>
	//	requires(std::is_void_v<std::void_t<decltype(unicode::ConvertU8String(std::declval<From>()))>>)
	inline	void	ConvertString(From&& str, std::span<To> dest_buffer)
	{
		unicode::ConvertU8String(str, dest_buffer);
	}


	template<std::same_as<nox::U16String> To, class From>
		requires(std::is_void_v<std::void_t<decltype(unicode::ConvertU16String(std::declval<From>()))>>)
	inline	To	ConvertString(From&& str)
	{
		return unicode::ConvertU16String(str);
	}

	/// @brief ConvertU16String span ver
	/// @tparam From 
	/// @tparam To 
	/// @param dest_buffer 
	/// @param str 
	template<class To, class From>
	//	requires(std::is_void_v<std::void_t<decltype(unicode::ConvertU16String(std::declval<From>()))>>)
	inline	void	ConvertString(From&& str, std::span<To> dest_buffer)
	{
		unicode::ConvertU16String(str, dest_buffer);
	}

	/// @brief ConvertU32String
	/// @tparam From 
	/// @tparam To 
	/// @param str 
	/// @return 
	template<std::same_as<nox::U32String> To, class From>
		requires(std::is_void_v<std::void_t<decltype(unicode::ConvertU32String(std::declval<From>()))>>)
		inline	To	ConvertString(From&& str)
	{
		return unicode::ConvertU32String(str);
	}

		/// @brief ConvertU32String span ver
		/// @tparam From 
		/// @tparam To 
		/// @param str 
		/// @return 
		template<std::same_as<char32> To, class From>
			//requires(std::is_void_v<std::void_t<decltype(unicode::ConvertU32String(std::declval<From>()))>>)
		inline	void	ConvertString(From&& str, std::span<To> dest_buffer)
		{
			unicode::ConvertU32String(str, dest_buffer);
		}
}