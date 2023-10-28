///	@file	point3d.h
///	@brief	point3d
#pragma once

#include	"point2d.h"

namespace nox
{
	namespace detail
	{
		/// @brief 2要素の基底型
		/// @tparam _ValueType 算術型
		template<concepts::Arithmetic _ValueType>
		struct Point3D
		{
			using ValueType = _ValueType;

			_ValueType x, y, z;

			//	コンストラクタ
			[[nodiscard]] constexpr Point3D() noexcept : x(.0f), y(.0f), z(.0f) {}
			[[nodiscard]] constexpr explicit Point3D(_ValueType _x, _ValueType _y, _ValueType _z)noexcept : x(_x), y(_y), z(_z) {}
			[[nodiscard]] constexpr	explicit Point3D(const std::array<_ValueType, 3>& ary) :x(ary[0]), y(ary[1]), z(ary[2]) {}
			[[nodiscard]] constexpr explicit Point3D(const _ValueType(&ary)[3])noexcept : x(ary[0]), y(ary[1]), z(ary[2]) {}
			[[nodiscard]] constexpr	Point3D(const Point3D<_ValueType>& _v)noexcept : x(_v.x), y(_v.y), z(_v.z) {}

			/// @brief Point2Dへのキャスト
			inline	Point2D<_ValueType>& CastVec2()noexcept { return *reinterpret_cast<Point2D<_ValueType>*>(this); }

			/// @brief Point2Dへのキャスト
			inline	const Point2D<_ValueType>& CastVec2()const noexcept { return *reinterpret_cast<const Point2D<_ValueType>*>(this); }

			/// @brief Point2Dへの変換
			[[nodiscard]] constexpr Point2D<_ValueType> ToVec2()const noexcept { return Point2D<_ValueType>(x, y); }

			/// @brief XZを取得
			[[nodiscard]] constexpr Point2D<_ValueType> GetXZ()const noexcept { return Point2D<_ValueType>(x, z); }

#pragma region operator

			[[nodiscard]] constexpr inline bool operator == (const Point3D<_ValueType>& _v) const noexcept { return x == _v.x && y == _v.y && z == _v.z; }
			[[nodiscard]] constexpr	inline bool operator != (const Point3D<_ValueType>& _v) const noexcept { return x != _v.x || y != _v.y || z != _v.z; }

			constexpr inline Point3D<_ValueType>& operator = (const Point3D<_ValueType>& _v)noexcept { x = _v.x; y = _v.y; z = _v.z; return *this; }
			constexpr inline Point3D<_ValueType>& operator += (const Point3D<_ValueType>& _v)noexcept { x += _v.x; y += _v.y; z += _v.z; return *this; }
			constexpr inline Point3D<_ValueType>& operator -= (const Point3D<_ValueType>& _v)noexcept { x -= _v.x; y -= _v.y; z -= _v.z; return *this; }
			constexpr inline Point3D<_ValueType>& operator *= (_ValueType _v)noexcept { x *= _v; y *= _v; z *= _v; return *this; }
			constexpr inline Point3D<_ValueType>& operator /= (_ValueType _v)noexcept { x /= _v; y /= _v; z /= _v; return *this; }

			constexpr inline Point3D<_ValueType> operator + () const noexcept { return Point3D<_ValueType>(x, y, z); }
			constexpr inline Point3D<_ValueType> operator - () const noexcept { return Point3D<_ValueType>(-x, -y, -z); }

			constexpr inline Point3D<_ValueType> operator + (const Point3D<_ValueType>& _v) const noexcept { return Point3D<_ValueType>(x + _v.x, y + _v.y, z + _v.z); }
			constexpr inline Point3D<_ValueType> operator - (const Point3D<_ValueType>& _v) const noexcept { return Point3D<_ValueType>(x - _v.x, y - _v.y, z - _v.z); }
			constexpr inline Point3D<_ValueType> operator * (_ValueType _v) const noexcept { return Point3D<_ValueType>(x * _v, y * _v, z * _v); }
			constexpr inline Point3D<_ValueType> operator / (_ValueType _v) const noexcept { return Point3D<_ValueType>(x / _v, y / _v, z / _v); }
#pragma endregion

		};
	}

	using Int3 = nox::detail::Point3D<s32>;
	using UInt3 = nox::detail::Point3D<u32>;
	using Float3 = nox::detail::Point3D<f32>;
	using Double3 = nox::detail::Point3D<f64>;
}