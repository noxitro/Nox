//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	math_algorithm.h
///	@brief	math_algorithm
#pragma once
#include	<cmath>
#include	<tuple>
#include	"../basic_type.h"
#include	"../type_traits/concepts.h"

#include	"math_definition.h"

namespace nox::math
{
	template<concepts::Arithmetic T>
	[[nodiscard]] inline	 T	Sin(T x)noexcept { return std::sin(x); }

	template<concepts::Arithmetic T>
	[[nodiscard]] inline	 T	Cos(T x)noexcept { return std::cos(x); }

	template<concepts::Arithmetic T>
	[[nodiscard]] inline	constexpr  T	Max(T a, T b)noexcept { return ((a) > (b)) ? (a) : (b); }

	template<concepts::Arithmetic T>
	[[nodiscard]]	inline	constexpr	T	Min(T a, T b)noexcept { return ((a) < (b)) ? (a) : (b); }

	template<concepts::Arithmetic T>
	[[nodiscard]] inline	constexpr T	Clamp(const T source, const T min, const T max)noexcept
	{
		if (min > source) return min;
		if (max < source) return max;
		return source;
	}

	template<std::integral T>
	inline constexpr T Pow(T base, T exp) noexcept
	{
		if (exp < 0)
		{
			return 0; // 整数型で負のべき乗は0（または1/base^|exp|だが整数では0）
		}
		T result = 1;
		while (exp)
		{
			if (exp & 1)
			{
				result *= base;
			}
			base *= base;
			exp >>= 1;
		}
		return result;
	}

	template<std::integral T>
	inline constexpr bool IsPow2(T value) noexcept
	{
		if (value <= 0)
		{
			return false;
		}
		return (value & (value - 1)) == 0;
	}

	/// @brief 0.0 ～ 1.0の間に収める
	/// @tparam T 浮動小数点型
	/// @param value 値
	/// @return 0.0 ～ 1.0の間に収められた値
	template<std::floating_point T>
	[[nodiscard]] inline	constexpr T Saturate(const T value)noexcept
	{
		if (value < static_cast<T>(0.0)) return static_cast<T>(0.0);
		if (value > static_cast<T>(1.0)) return static_cast<T>(1.0);
		return value;
	}

	template<concepts::Arithmetic T>
	[[nodiscard]] inline	constexpr	T	Lerp(const T a, const T b, const T t)noexcept
	{
		return a + (b - a) * t;
	}

	/// @brief .0 ～ 1.0の間に補間する
	/// @tparam T 
	/// @param a 
	/// @param b 
	/// @param source 補間する値
	/// @return 補間された値
	template<concepts::Arithmetic T>
	[[nodiscard]]
	inline	constexpr T	InverseLerp(T a, T b, T source)noexcept
	{
		return (source - b) / (a - b);
	}

	/// @brief 度数法の値から弧度法の値へ変換する
	/// @param degree 度数法の値
	/// @return 弧度法の値
	template<std::floating_point T>
	[[nodiscard]]
	inline	constexpr	T DegreeToRadian(const T degree)noexcept { return degree * nox::math::PI<T> / static_cast<T>(180.0); }

	/// @brief 弧度法の値から度数法の値へ変換する
	/// @param radian 弧度法の値
	template<std::floating_point T>
	[[nodiscard]]
	inline	constexpr	T RadianToDegree(const T radian)noexcept { return radian * static_cast<T>(180.0) / nox::math::PI<T>; }

	/// @brief 整数の割り算と剰余を同時に計算する
	/// @tparam T 整数値の型 
	/// @param dividend 
	/// @param divisor 
	/// @return 
	template<std::integral T>
	[[nodiscard]] inline constexpr std::pair<T, T> DivRem(T dividend, T divisor) noexcept
	{
		return { dividend / divisor, dividend % divisor };
	}
}