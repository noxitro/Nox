// Copyright (C) 2026 NOX ENGINE All rights reserved.

///	@file	hash.h
///	@brief	hash
#pragma once
#include	<cstdint>
#include	<string_view>
#include	<xhash>
namespace nox
{
	template<class T>
	inline constexpr std::uint32_t Crc32(std::basic_string_view<T> str)
	{
		std::uint32_t crc = 0xFFFFFFFF;
		for (auto c : str)
		{
			crc ^= static_cast<std::uint8_t>(c);
			for (int i = 0; i < 8; ++i)
			{
				crc = (crc >> 1) ^ (0xEDB88320 & -(crc & 1));
			}
		}
		return ~crc;
	}
}