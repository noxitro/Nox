///	@file	quaternion.h
///	@brief	quaternion
#pragma once

#include	"vector4d.h"

namespace nox
{
	namespace detail
	{
		template<concepts::Arithmetic _ValueType>
		struct Quaternion
		{
			_ValueType x, y, z, w;

			[[nodiscard]]	inline	constexpr	Quaternion()noexcept : x((_ValueType)0), y((_ValueType)0), z((_ValueType)0), w((_ValueType)1) {}
			[[nodiscard]]	inline	constexpr explicit	Quaternion(_ValueType _x, _ValueType _y, _ValueType _z, _ValueType _w)noexcept : x(_x), y(_y), z(_z), w(_w) {}
			[[nodiscard]]	inline	constexpr	Quaternion(const Quaternion& q)noexcept : x(q.x), y(q.y), z(q.z), w(q.w) {}
			[[nodiscard]]	inline	constexpr explicit	Quaternion(const std::array<_ValueType, 4U>& ary)noexcept :
				x(ary[0]), y(ary[1]), z(ary[2]), w(ary[3]) {}
			[[nodiscard]]	inline	constexpr explicit	Quaternion(const _ValueType(&ary)[4])noexcept :
				x(ary[0]), y(ary[1]), z(ary[2]), w(ary[3]) {}

			//------------------------------------------------------
			//	operator
			//------------------------------------------------------
			[[nodiscard]]	constexpr inline bool operator == (const Quaternion& _q) const noexcept { return x == _q.x && y == _q.y && z == _q.z && w == _q.w; }
			[[nodiscard]]	constexpr	inline bool operator != (const Quaternion& _q) const noexcept { return x != _q.x || y != _q.y || z != _q.z || w != _q.w; }

			[[nodiscard]] inline constexpr Quaternion operator + () const noexcept { return Quaternion(x, y, z, w); }
			[[nodiscard]]	inline constexpr Quaternion operator - () const noexcept { return Quaternion(-x, -y, -z, -w); }
			[[nodiscard]]	inline	constexpr Quaternion operator *(_ValueType v) const noexcept { return Quaternion(x * v, y * v, z * v, w * v); }
			[[nodiscard]]	inline	constexpr Quaternion operator /(_ValueType v) const noexcept { return Quaternion(x / v, y / v, z / v, w / v); }

			inline	constexpr Quaternion& operator *=(_ValueType v)noexcept { x *= v; y *= v; z *= v; w *= v; return *this; }
			inline	constexpr Quaternion& operator /=(_ValueType v)noexcept { x /= v; y /= v; z /= v; w /= v; return *this; }

			[[nodiscard]]	inline	_ValueType operator[](const u32 index)const noexcept { return reinterpret_cast<const _ValueType*>(this)[index]; }
			[[nodiscard]]	inline	_ValueType& operator[](const u32 index)noexcept { return reinterpret_cast<_ValueType*>(this)[index]; }

			inline	constexpr Quaternion& operator =(const Quaternion& q) noexcept {
				x = q.x; y = q.y; z = q.z; w = q.w;
				return *this;
			}

			inline	constexpr	Quaternion& operator *=(const Quaternion& q)noexcept {
				x = w * q.x + x * q.w + z * q.y - y * q.z;
				y = w * q.y + y * q.w + x * q.z - z * q.x;
				z = w * q.z + z * q.w + y * q.x - x * q.y;
				w = w * q.w - x * q.x - y * q.y - z * q.z;
				return *this;
			}

			[[nodiscard]]	constexpr Quaternion operator * (const Quaternion& q)const noexcept {
				return Quaternion(
					w * q.x + x * q.w + z * q.y - y * q.z,
					w * q.y + y * q.w + x * q.z - z * q.x,
					w * q.z + z * q.w + y * q.x - x * q.y,
					w * q.w - x * q.x - y * q.y - z * q.z
				);
			}
		};
	}

	using Quat = nox::detail::Quaternion<f32>;
}