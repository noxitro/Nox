///	@file	color.h
///	@brief	color
#pragma once
#include	"../basic_type.h"
#include	<array>

namespace nox
{
	struct Color
	{
		uint8 r;
		uint8 g;
		uint8 b;
		uint8 a;

		[[nodiscard]]	inline constexpr Color()noexcept :
			r(0U), g(0U), b(0U), a(0U) {}

		[[nodiscard]]	inline constexpr Color(const Color& _v)noexcept :
			r(_v.r), g(_v.g), b(_v.b), a(_v.a) {}

		template<std::floating_point T>
		[[nodiscard]]	inline constexpr explicit Color(T _r, T _g, T _b, T _a)noexcept :
			r(static_cast<uint8>(_r * static_cast<T>(255.0))),
			g(static_cast<uint8>(_g * static_cast<T>(255.0))),
			b(static_cast<uint8>(_b * static_cast<T>(255.0))),
			a(static_cast<uint8>(_a * static_cast<T>(255.0))) {}

		[[nodiscard]] inline constexpr explicit Color(uint8 _r, uint8 _g, uint8 _b, uint8 _a)noexcept :
			r(_r), g(_g), b(_b), a(_a) {}

		[[nodiscard]] inline constexpr explicit Color(const std::array<uint8, 4U>& _ary) :
			r(_ary[0]), g(_ary[1]), b(_ary[2]), a(_ary[3]) {}

		[[nodiscard]]	inline constexpr bool operator == (const Color& _v) const noexcept { return r == _v.r && g == _v.g && b == _v.b && a == _v.a; }

		inline constexpr Color& operator = (const Color& _v)noexcept 
		{
			r = _v.r;
			g = _v.g;
			b = _v.b;
			a = _v.a;
			return *this;
		}

	};
}