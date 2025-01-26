//	Copyright (C) 2024 NOX ENGINE All Rights Reserved.

///	@file	math_algorithm.h
///	@brief	math_algorithm
#pragma once
#include	"../basic_type.h"

#include	<cmath>
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
	[[nodiscard]]	inline	constexpr	bool	Min(T a, T b)noexcept { return ((a) < (b)) ? (a) : (b); }

	template<concepts::Arithmetic T>
	[[nodiscard]] inline	constexpr T	Clamp(const T source, const T min, const T max)noexcept
	{
		if (min > source) return min;
		if (max < source) return max;
		return source;
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
}