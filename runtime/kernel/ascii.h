//	Copyright (C) 2026 NOX ENGINE All rights reserved.

///	@file	ascii.h
///	@brief	ascii
#pragma once
#include	"advanced_type.h"

namespace nox::encoding::ascii
{
	template<typename T>
	inline constexpr bool IsAsciiString(std::basic_string_view<T> str)noexcept
	{
		for (const T ch : str)
		{
			if (ch > static_cast<T>(0x7F))
			{
				return false;
			}
		}
		return true;
	}

	template<typename To, typename From> requires(std::is_same_v<To, From> == false)
	inline constexpr std::basic_string_view<To> ConvertString(std::basic_string_view<From> str, std::span<To> dest_buffer) noexcept
	{
		const size_t copy_size = str.size() < dest_buffer.size() ? str.size() : dest_buffer.size();
		for (size_t i = 0; i < copy_size; ++i)
		{
			dest_buffer[i] = static_cast<To>(str[i]);
		}
		return std::basic_string_view<To>(dest_buffer.data(), copy_size);
	}

	template<typename To, typename From> requires(std::is_same_v<To, From> == false)
		inline constexpr std::optional<std::basic_string_view<To>> TryConvertString(std::basic_string_view<From> str, std::span<To> dest_buffer) noexcept
	{
		if (nox::encoding::ascii::IsAsciiString(str) == false)
		{
			return std::nullopt;
		}
		return ConvertString<To, From>(str, dest_buffer);
	}

	template<typename To, typename From> requires(std::is_same_v<To, From> == false)
		inline constexpr nox::StlBasicString<To> ConvertString(std::basic_string_view<From> str)
	{
		nox::StlBasicString<To> result;
		result.resize(str.size());

		nox::encoding::ascii::ConvertString<To, From>(str, std::span<To>(result.data(), result.size()));
		return result;
	}
}