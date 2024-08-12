///	@file	vector3d.h
///	@brief	vector3d
#pragma once

#include	"vector2d.h"
#include	"point3d.h"

namespace nox
{
	namespace detail
	{
		template<concepts::Arithmetic _ValueType>
		struct Vector3D
		{
			using ValueType = _ValueType;

			_ValueType x;
			_ValueType y;
			_ValueType z;
		private:
			const _ValueType _padding = 0;
		public:
			[[nodiscard]]	inline	static	constexpr	Vector3D<_ValueType>	Zero()noexcept { return Vector3D<_ValueType>(); }
			[[nodiscard]]	inline	static	constexpr	Vector3D<_ValueType>	One()noexcept { return Vector3D<_ValueType>(1, 1, 1); }
			[[nodiscard]]	inline	static	constexpr	Vector3D<_ValueType>	AxisX()noexcept { return Vector3D<_ValueType>(1, 0, 0); }
			[[nodiscard]]	inline	static	constexpr	Vector3D<_ValueType>	AxisY()noexcept { return Vector3D<_ValueType>(0, 1, 0); }
			[[nodiscard]]	inline	static	constexpr	Vector3D<_ValueType>	AxisZ()noexcept { return Vector3D<_ValueType>(0, 0, 1); }

			//	コンストラクタ
			[[nodiscard]]	constexpr Vector3D() noexcept :
				x(static_cast<_ValueType>(0)), y(static_cast<_ValueType>(0)), z(static_cast<_ValueType>(0)) {}

			[[nodiscard]] constexpr explicit Vector3D(_ValueType _x, _ValueType _y, _ValueType _z)noexcept :
				x(_x), y(_y), z(_z) {}

			[[nodiscard]]	constexpr	explicit Vector3D(const std::array<_ValueType, 3>& ary) :
				x(ary[0]), y(ary[1]), z(ary[2]) {}

			[[nodiscard]]	constexpr explicit Vector3D(const _ValueType(&ary)[3])noexcept :
				x(ary[0]), y(ary[1]), z(ary[2]) {}

			[[nodiscard]]	constexpr	Vector3D(const Vector3D& _v)noexcept :
				x(_v.x), y(_v.y), z(_v.z) {}


			//!<@brief	Vector2D<_ValueType>へのキャスト
			[[nodiscard]]	inline Vector2D<_ValueType>& CastVector2D()noexcept { return *reinterpret_cast<Vector2D<_ValueType>*>(this); }
			//!<@brief	Vector2D<_ValueType>へのキャストconstVer
			[[nodiscard]]	inline const Vector2D<_ValueType>& CastVector2D()const noexcept { return *reinterpret_cast<const Vector2D<_ValueType>*>(this); }
			//!<@brief	Vector2D<_ValueType>への変換
			[[nodiscard]]	constexpr Vector2D<_ValueType> ToVector2D()const noexcept { return Vector2D<_ValueType>(x, y); }
			//!<@brief	XZを取得
			[[nodiscard]]	constexpr Vector2D<_ValueType> GetXZ()const noexcept { return Vector2D<_ValueType>(x, z); }

			//!<@brief	配列へのキャスト
			[[nodiscard]]		constexpr inline	std::array<_ValueType*, 3> CastArray() { return std::array<_ValueType*, 3>{&x, & y, & z}; }

			//!<@brief	配列への変換
			[[nodiscard]]	constexpr	inline	std::array<_ValueType, 3> ToArray()const { return std::array<_ValueType, 3>{x, y, z}; }


			//------------------------------------------------------
			//	operator
			//------------------------------------------------------
			[[nodiscard]]	constexpr inline bool operator == (const Vector3D& _v) const noexcept { return x == _v.x && y == _v.y && z == _v.z; }
			[[nodiscard]]	constexpr	inline bool operator != (const Vector3D& _v) const noexcept { return x != _v.x || y != _v.y || z != _v.z; }

			constexpr inline Vector3D& operator = (const Vector3D& _v)noexcept { x = _v.x; y = _v.y; z = _v.z; return *this; }
			constexpr inline Vector3D& operator += (const Vector3D& _v)noexcept { x += _v.x; y += _v.y; z += _v.z; return *this; }
			constexpr inline Vector3D& operator -= (const Vector3D& _v)noexcept { x -= _v.x; y -= _v.y; z -= _v.z; return *this; }
			constexpr inline Vector3D& operator *= (_ValueType _v)noexcept { x *= _v; y *= _v; z *= _v; return *this; }
			constexpr inline Vector3D& operator /= (_ValueType _v)noexcept { x /= _v; y /= _v; z /= _v; return *this; }

			constexpr inline Vector3D operator + () const noexcept { return Vector3D(x, y, z); }
			constexpr inline Vector3D operator - () const noexcept { return Vector3D(-x, -y, -z); }

			constexpr inline Vector3D operator + (const Vector3D& _v) const noexcept { return Vector3D(x + _v.x, y + _v.y, z + _v.z); }
			constexpr inline Vector3D operator - (const Vector3D& _v) const noexcept { return Vector3D(x - _v.x, y - _v.y, z - _v.z); }
			constexpr inline Vector3D operator * (_ValueType _v) const noexcept { return Vector3D(x * _v, y * _v, z * _v); }
			constexpr inline Vector3D operator / (_ValueType _v) const noexcept { return Vector3D(x / _v, y / _v, z / _v); }
		};
	}

	using Vec3 = nox::detail::Vector3D<nox::float_t>;
}