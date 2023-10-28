///	@file	color.h
///	@brief	color
#pragma once
#include	"../if/basic_type.h"
#include	<array>

namespace nox
{
	struct Color
	{
		u8 r;
		u8 g;
		u8 b;
		u8 a;

		[[nodiscard]]	inline constexpr Color()noexcept :
			r(0U), g(0U), b(0U), a(0U) {}

		[[nodiscard]]	inline constexpr Color(const Color& _v)noexcept :
			r(_v.r), g(_v.g), b(_v.b), a(_v.a) {}

		[[nodiscard]]	inline constexpr explicit Color(f32 _r, f32 _g, f32 _b, f32 _a)noexcept :
			r(static_cast<u8>(_r * 255.0f)),
			g(static_cast<u8>(_g * 255.0f)),
			b(static_cast<u8>(_b * 255.0f)),
			a(static_cast<u8>(_a * 255.0f)) {}

		[[nodiscard]]	inline constexpr explicit Color(f64 _r, f64 _g, f64 _b, f64 _a)noexcept :
			r(static_cast<u8>(_r * 255.0)),
			g(static_cast<u8>(_g * 255.0)),
			b(static_cast<u8>(_b * 255.0)),
			a(static_cast<u8>(_a * 255.0)) {}

		[[nodiscard]] inline constexpr explicit Color(u8 _r, u8 _g, u8 _b, u8 _a)noexcept :
			r(_r), g(_g), b(_b), a(_a) {}

		[[nodiscard]]	inline constexpr explicit Color(const u8(&_ary)[4])noexcept :
			r(_ary[0]), g(_ary[1]), b(_ary[2]), a(_ary[3]) {}

		[[nodiscard]] inline constexpr explicit Color(const std::array<u8, 4U>& _ary) :
			r(_ary[0]), g(_ary[1]), b(_ary[2]), a(_ary[3]) {}

		//	[[nodiscard]]	inline constexpr Color ToColor()const noexcept { return Color(r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f); }

			//------------------------------------------------------
			//	operator
			//------------------------------------------------------
		[[nodiscard]]	inline constexpr bool operator == (const Color& _v) const noexcept { return r == _v.r && g == _v.g && b == _v.b && a == _v.a; }
		[[nodiscard]]	inline constexpr bool operator != (const Color& _v) const noexcept { return r != _v.r || g != _v.g || b != _v.b || a != _v.a; }

		inline constexpr Color& operator = (const Color& _v)noexcept {
			r = _v.r;
			g = _v.g;
			b = _v.b;
			a = _v.a;
			return *this;
		}

	};
}