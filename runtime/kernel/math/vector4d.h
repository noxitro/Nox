///	@file	vector4d.h
///	@brief	vector4d
#pragma once
#include	"vector3d.h"
#include	"point4d.h"

namespace nox
{
	namespace detail
	{
		template<concepts::Arithmetic _ValueType>
		struct Vector4D
		{
			using ValueType = _ValueType;

			_ValueType x, y, z, w;


			constexpr Vector4D() noexcept : x(.0f), y(.0f), z(.0f), w(.0f) {}

			constexpr explicit Vector4D(_ValueType _x, _ValueType _y, _ValueType _z, _ValueType _w)noexcept : x(_x), y(_y), z(_z), w(_w) {}

			constexpr explicit Vector4D(const _ValueType* pArray)noexcept : x(pArray[0]), y(pArray[1]), z(pArray[2]), w(pArray[3]) {}

			constexpr explicit Vector4D(const std::array<_ValueType, 4>& ary)noexcept : x(ary[0]), y(ary[1]), z(ary[2]), w(ary[3]) {}

			constexpr	explicit Vector4D(const Vec3& _xyz, _ValueType _w)noexcept :x(_xyz.x), y(_xyz.y), z(_xyz.z), w(_w) {}

			constexpr	explicit Vector4D(const Point4D<_ValueType>& _v)noexcept : x(_v.x), y(_v.y), z(_v.z), w(_v.w) {}

			constexpr	explicit	Vector4D(const Vector4D<_ValueType>& _v)noexcept : x(_v.x), y(_v.y), z(_v.z), w(_v.w) {}


			//!<@brief	Vec2へのキャスト
			[[nodiscard]]	inline Vector2D<_ValueType>& CastVec2()noexcept { return *reinterpret_cast<Vector2D<_ValueType>*>(this); }
			[[nodiscard]]	const Vector2D<_ValueType>& CastVec2()const noexcept { return *reinterpret_cast<const Vector2D<_ValueType>*>(this); }
			//!<@brief	Vec3へのキャスト
			[[nodiscard]]	inline Vector3D<_ValueType>& CastVec3()noexcept { return *reinterpret_cast<Vector3D<_ValueType>*>(this); }
			[[nodiscard]] const Vector3D<_ValueType>& CastVec3()const noexcept { return *reinterpret_cast<const Vector3D<_ValueType>*>(this); }

			[[nodiscard]]	constexpr Vector2D<_ValueType> ToVec2()const { return Vector2D<_ValueType>(x, y); }
			[[nodiscard]]	constexpr Vec3 ToVec3()const { return Vector3D<_ValueType>(x, y, z); }

			[[nodiscard]]	constexpr std::array<_ValueType, 4> ToArray()const noexcept { return std::array<_ValueType, 4>{x, y, z, w}; }

			//	オペレータ
			[[nodiscard]]	constexpr bool operator == (const Vector4D& _v) const noexcept { return x == _v.x && y == _v.y && z == _v.z && w == _v.w; }
			[[nodiscard]]	constexpr bool operator != (const Vector4D& _v) const noexcept { return x != _v.x || y != _v.y || z != _v.z || w != _v.w; }

			constexpr Vector4D& operator = (const Vector4D& _v)noexcept { x = _v.x; y = _v.y; z = _v.z; w = _v.w; return *this; }
			constexpr Vector4D& operator += (const Vector4D& _v) noexcept { x += _v.x; y += _v.y; z += _v.z; w += _v.w; return *this; }
			constexpr Vector4D& operator -= (const Vector4D& _v) noexcept { x -= _v.x; y -= _v.y; z -= _v.z; w -= _v.w; return *this; }
			constexpr Vector4D& operator *= (_ValueType _v)noexcept { x *= _v; y *= _v; z *= _v; w *= _v; return *this; }
			constexpr Vector4D& operator /= (_ValueType _v) noexcept { x /= _v; y /= _v; z /= _v; w /= _v; return *this; }

			[[nodiscard]]	constexpr	Vector4D operator + () const noexcept { return Vector4D(x, y, z, w); }
			[[nodiscard]]	constexpr	Vector4D operator - () const noexcept { return Vector4D(-x, -y, -z, -w); }

			[[nodiscard]]	constexpr Vector4D operator + (const Vector4D& _v) const noexcept { return Vector4D(x + _v.x, y + _v.y, z + _v.z, w + _v.w); }
			[[nodiscard]]	constexpr Vector4D operator - (const Vector4D& _v) const noexcept { return Vector4D(x - _v.x, y - _v.y, z - _v.z, w - _v.w); }
			[[nodiscard]]	constexpr Vector4D operator * (_ValueType _v) const noexcept { return Vector4D(x * _v, y * _v, z * _v, w * _v); }
			[[nodiscard]]	constexpr Vector4D operator / (_ValueType _v) const noexcept { return Vector4D(x / _v, y / _v, z / _v, w / _v); }

			inline	_ValueType operator[](const s32 index)const noexcept { return reinterpret_cast<const _ValueType*>(this)[index]; }
			inline	_ValueType& operator[](const s32 index)noexcept { return reinterpret_cast<_ValueType*>(this)[index]; }
		};

	}

	using Vec4 = nox::detail::Vector4D<f32>;
}