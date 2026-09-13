// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

///	@file	encoding.h
///	@brief	encoding
#pragma once
#include	<span>
#include	<string_view>
#include	<optional>

namespace nox
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

	template<typename To, typename From, size_t _Size = std::dynamic_extent> requires(std::is_same_v<To, From> == false)
		inline constexpr std::basic_string_view<To> ConvertString(std::basic_string_view<From> str, std::span<To, _Size> dest_buffer) noexcept
	{
		const size_t copy_size = str.size() < dest_buffer.size() ? str.size() : dest_buffer.size();
		for (size_t i = 0; i < copy_size; ++i)
		{
			dest_buffer[i] = static_cast<To>(str[i]);
		}
		return std::basic_string_view<To>(dest_buffer.data(), copy_size);
	}

	template<typename To, typename From, size_t _Size = std::dynamic_extent> requires(std::is_same_v<To, From> == false)
		inline constexpr std::optional<std::basic_string_view<To>> TryConvertString(std::basic_string_view<From> str, std::span<To, _Size> dest_buffer) noexcept
	{
		if (nox::IsAsciiString(str) == false)
		{
			return std::nullopt;
		}
		return ConvertString<To, From>(str, dest_buffer);
	}
}