// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

///	@file	f16.h
///	@brief	f16
#pragma once
#include	<cstdint>
#include	<cfloat>
#include	<limits>

namespace nox
{
	struct Float16
	{
		union
		{
			struct
			{
				std::uint16_t fraction : 10;
				std::uint16_t exponent : 5;
				std::uint16_t sign : 1;
			};
			std::uint16_t bits;
		};
		constexpr Float16() noexcept : bits(0) {}
		constexpr explicit Float16(float value) noexcept
		{
			const std::uint32_t float_bits = *reinterpret_cast<const std::uint32_t*>(&value);
			const std::uint32_t sign = (float_bits >> 31) & 0x1;
			const std::int32_t exponent = ((float_bits >> 23) & 0xFF) - 127 + 15;
			const std::uint32_t fraction = (float_bits >> 13) & 0x3FF;
			if (exponent <= 0)
			{
				bits = static_cast<std::uint16_t>(sign << 15);
			}
			else if (exponent >= 31)
			{
				bits = static_cast<std::uint16_t>((sign << 15) | (0x1F << 10));
			}
			else
			{
				bits = static_cast<std::uint16_t>((sign << 15) | (exponent << 10) | fraction);
			}
		}
		constexpr operator float() const noexcept
		{
			const std::uint32_t sign = (bits >> 15) & 0x1;
			const std::int32_t exponent = ((bits >> 10) & 0x1F) - 15 + 127;
			const std::uint32_t fraction = bits & 0x3FF;
			if (exponent <= 0)
			{
				return sign ? -0.0f : 0.0f;
			}
			else if (exponent >= 31)
			{
				return sign ? -std::numeric_limits<float>::infinity() : std::numeric_limits<float>::infinity();
			}
			else
			{
				std::uint32_t float_bits = (sign << 31) | (exponent << 23) | (fraction << 13);
				return *reinterpret_cast<float*>(&float_bits);
			}
		}
	};
}