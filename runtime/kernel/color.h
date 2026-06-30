///	@file	color.h
///	@brief	color
#pragma once
#include	<array>
#include	"basic_type.h"

namespace nox
{
	struct Color
	{
		nox::uint8 r;
		nox::uint8 g;
		nox::uint8 b;
		nox::uint8 a;

		[[nodiscard]]	inline constexpr Color()noexcept :
			r(0U), g(0U), b(0U), a(0U) {}

		template<std::floating_point T>
		[[nodiscard]]	inline constexpr explicit Color(T _r, T _g, T _b, T _a)noexcept :
			r(static_cast<nox::uint8>(_r * static_cast<T>(255.0))),
			g(static_cast<nox::uint8>(_g * static_cast<T>(255.0))),
			b(static_cast<nox::uint8>(_b * static_cast<T>(255.0))),
			a(static_cast<nox::uint8>(_a * static_cast<T>(255.0))) {}

		[[nodiscard]] inline constexpr explicit Color(nox::uint8 _r, nox::uint8 _g, nox::uint8 _b, nox::uint8 _a)noexcept :
			r(_r), g(_g), b(_b), a(_a) {}

		[[nodiscard]] inline constexpr explicit Color(const std::array<nox::uint8, 4U>& _ary) :
			r(_ary[0]), g(_ary[1]), b(_ary[2]), a(_ary[3]) {}

		[[nodiscard]]	inline constexpr bool operator == (const Color& _v) const noexcept { return r == _v.r && g == _v.g && b == _v.b && a == _v.a; }
	};

	static_assert(std::is_trivially_copyable_v<Color>, "Color must be trivially copyable");
}