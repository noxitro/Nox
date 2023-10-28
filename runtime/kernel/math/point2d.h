///	@file	point2d.h
///	@brief	point2d
#pragma once

#include	"../if/basic_type.h"
#include	"../type_traits/concepts.h"

#include	"../if/basic_definition.h"

namespace nox
{
	namespace detail
	{
		/// @brief 2要素の基底型
		/// @tparam _ValueType 算術型
		template<concepts::Arithmetic _ValueType>
		struct Point2D
		{
			using ValueType = _ValueType;

			_ValueType x, y;

			//	コンストラクタ
			[[nodiscard]] constexpr Point2D() noexcept :
				x(0), y(0) {}

			[[nodiscard]] constexpr explicit Point2D(_ValueType _x, _ValueType _y)noexcept :
				x(_x), y(_y) {}

			[[nodiscard]] constexpr	explicit Point2D(const std::array<_ValueType, 2>& ary) :
				x(ary[0]), y(ary[1]) {}

			[[nodiscard]] constexpr explicit Point2D(const _ValueType(&ary)[2])noexcept :
				x(ary[0]), y(ary[1]) {}

			[[nodiscard]] constexpr	Point2D(const Point2D& _v)noexcept :
				x(_v.x), y(_v.y) {}

			//------------------------------------------------------
			//	operator
			//------------------------------------------------------
			[[nodiscard]] inline constexpr bool operator==(const Point2D& _v)const noexcept
			{
				return x == _v.x && y == _v.y;
			}
			[[nodiscard]] inline constexpr bool operator!=(const Point2D& _v)const noexcept
			{
				return x != _v.x || y != _v.y;
			}

			[[nodiscard]] constexpr Point2D operator + () const noexcept { return Point2D(x, y); }
			[[nodiscard]] constexpr Point2D operator - () const noexcept { return Point2D(-x, -y); }

			[[nodiscard]] constexpr Point2D operator + (const Point2D& _v) const noexcept { return Point2D(x + _v.x, y + _v.y); }
			[[nodiscard]] constexpr Point2D operator - (const Point2D& _v) const noexcept { return Point2D(x - _v.x, y - _v.y); }
			[[nodiscard]] constexpr Point2D operator * (_ValueType _v) const noexcept { return Point2D(x * _v, y * _v); }
			[[nodiscard]] constexpr Point2D operator / (_ValueType _v) const noexcept { return Point2D(x / _v, y / _v); }

			inline constexpr Point2D& operator = (const Point2D& _v)noexcept { x = _v.x; y = _v.y; return *this; }
			inline constexpr Point2D& operator += (const Point2D& _v)noexcept { x += _v.x; y += _v.y;  return *this; }
			inline constexpr Point2D& operator -= (const Point2D& _v)noexcept { x -= _v.x; y -= _v.y;  return *this; }
			inline constexpr Point2D& operator *= (_ValueType _v)noexcept { x *= _v; y *= _v; return *this; }
			inline constexpr Point2D& operator /= (_ValueType _v)noexcept { x /= _v; y /= _v; return *this; }
		};
	}

	using Int2 = nox::detail::Point2D<s32>;
	using UInt2 = nox::detail::Point2D<u32>;
	using Float2 = nox::detail::Point2D<f32>;
	using Double2 = nox::detail::Point2D<f64>;
}