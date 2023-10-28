///	@file	vector2d.h
///	@brief	vector2d
#pragma once
#include	"point2d.h"

namespace nox
{
	namespace detail
	{
		template<concepts::Arithmetic _ValueType>
		struct Vector2D
		{
			using ValueType = _ValueType;

			_ValueType x, y;

			//	コンストラクタ
			constexpr	Vector2D() noexcept :
				x(.0f), y(.0f) {}

			constexpr explicit Vector2D(_ValueType _x, _ValueType _y)noexcept :
				x(_x), y(_y) {}

			constexpr	Vector2D(const Vector2D& _v)noexcept :
				x(_v.x), y(_v.y) {}

			constexpr explicit Vector2D(const f32* pArray)noexcept :
				x(pArray[0]), y(pArray[1]) {}

			//	オペレータ
			[[nodiscard]] constexpr bool operator == (const Vector2D& _v) const noexcept { return x == _v.x && y == _v.y; }
			[[nodiscard]] constexpr	bool operator != (const Vector2D& _v) const noexcept { return x != _v.x || y != _v.y; }

			constexpr Vector2D& operator = (const Vector2D& _v)noexcept { x = _v.x; y = _v.y; return *this; }
			constexpr Vector2D& operator += (const Vector2D& _v)noexcept { x += _v.x; y += _v.y;  return *this; }
			constexpr Vector2D& operator -= (const Vector2D& _v)noexcept { x -= _v.x; y -= _v.y;  return *this; }
			constexpr Vector2D& operator *= (_ValueType _v)noexcept { x *= _v; y *= _v; return *this; }
			constexpr Vector2D& operator /= (_ValueType _v)noexcept { x /= _v; y /= _v; return *this; }

			constexpr Vector2D operator + () const noexcept { return Vector2D(x, y); }
			constexpr Vector2D operator - () const noexcept { return Vector2D(-x, -y); }

			constexpr Vector2D operator + (const Vector2D& _v) const noexcept { return Vector2D(x + _v.x, y + _v.y); }
			constexpr Vector2D operator - (const Vector2D& _v) const noexcept { return Vector2D(x - _v.x, y - _v.y); }
			constexpr Vector2D operator * (_ValueType _v) const noexcept { return Vector2D(x * _v, y * _v); }
			constexpr Vector2D operator / (_ValueType _v) const noexcept { return Vector2D(x / _v, y / _v); }

			inline	_ValueType operator[](const _ValueType index)const noexcept { return reinterpret_cast<const _ValueType*>(this)[index]; }
			inline	_ValueType& operator[](const _ValueType index) noexcept { return reinterpret_cast<_ValueType*>(this)[index]; }
		};
	}

	using Vec2 = nox::detail::Vector2D<f32>;
}