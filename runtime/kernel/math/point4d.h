///	@file	point4d.h
///	@brief	point4d
#pragma once
#include	"point3d.h"

namespace nox
{
	namespace detail
	{
		template<concepts::Arithmetic _ValueType>
		struct Point4D
		{
			_ValueType x, y, z, w;

			//	コンストラクタ
			[[nodiscard]]	constexpr Point4D() noexcept :
				x(.0f), y(.0f), z(.0f), w(.0f) {}

			[[nodiscard]] constexpr explicit Point4D(_ValueType _x, _ValueType _y, _ValueType _z, _ValueType _w)noexcept :
				x(_x), y(_y), z(_z), w(_w) {}

			[[nodiscard]]	constexpr	explicit Point4D(const std::array<_ValueType, 4>& ary) :
				x(ary.at(0)), y(ary.at(1)), z(ary.at(2)), w(ary.at(3)) {}

			[[nodiscard]]	constexpr explicit Point4D(const _ValueType(&ary)[4])noexcept :
				x(ary[0]), y(ary[1]), z(ary[2]), w(ary[3]) {}

			[[nodiscard]]	constexpr	Point4D(const Point4D& _v)noexcept :
				x(_v.x), y(_v.y), z(_v.z), w(_v.w) {}

			//------------------------------------------------------
			//	operator
			//------------------------------------------------------
			[[nodiscard]] constexpr Point4D& operator==(const Point4D&)const noexcept = delete;
			constexpr Point4D& operator = (const Point4D& _v)noexcept { x = _v.x; y = _v.y; z = _v.z; w = _v.w; return *this; }
		};

	}

	using Int4 = nox::detail::Point4D<s32>;
	using UInt4 = nox::detail::Point4D<u32>;
	using Float4 = nox::detail::Point4D<f32>;
	using Double4 = nox::detail::Point4D<f64>;
}