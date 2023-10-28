///	@file	point4x4.h
///	@brief	point4x4
#pragma once
#include	"point4d.h"
#include	"matrix4d.h"

#pragma warning(push)
#pragma warning(disable:4201)
namespace nox
{
	namespace detail
	{
		template<concepts::Arithmetic _ValueType>
		struct Point4x4
		{
			union
			{
				struct 
				{
					_ValueType _11, _12, _13, _14;
					_ValueType _21, _22, _23, _24;
					_ValueType _31, _32, _33, _34;
					_ValueType _41, _42, _43, _44;
				}m;
				std::array<Point4D<_ValueType>, 4U> v;
			};

			[[nodiscard]]	constexpr Point4x4()noexcept :
				m{	1.0f, .0f, .0f, .0f,
					.0f, 1.0f, .0f, .0f,
					.0f, .0f, 1.0f, .0f,
					.0f, .0f, .0f, 1.0f } {}

			[[nodiscard]]	inline constexpr explicit Point4x4(
				_ValueType m00, _ValueType m01, _ValueType m02, _ValueType m03,
				_ValueType m10, _ValueType m11, _ValueType m12, _ValueType m13,
				_ValueType m20, _ValueType m21, _ValueType m22, _ValueType m23,
				_ValueType m30, _ValueType m31, _ValueType m32, _ValueType m33)noexcept :
				m{ m00,  m01, m02, m03,
					m10, m11, m12, m13,
					m20, m21, m22, m23,
					m30, m31, m32, m33 } {}

			//------------------------------------------------------
			//	operator
			//------------------------------------------------------
			[[nodiscard]] constexpr Point4x4& operator==(const Point4x4&)noexcept = delete;
			inline	constexpr Point4x4& operator = (const Point4x4& mat)noexcept {
				m._11 = mat.m._11; m._12 = mat.m._12; m._13 = mat.m._13; m._14 = mat.m._14;
				m._21 = mat.m._21; m._22 = mat.m._22; m._23 = mat.m._23; m._24 = mat.m._24;
				m._31 = mat.m._31; m._32 = mat.m._32; m._33 = mat.m._33; m._34 = mat.m._34;
				m._41 = mat.m._41; m._42 = mat.m._42; m._43 = mat.m._43; m._44 = mat.m._44;
				return *this;
			}

			inline constexpr Point4x4& operator = (const Matrix4D<_ValueType>& mat)noexcept {
				m._11 = mat.m._11; m._12 = mat.m._12; m._13 = mat.m._13; m._14 = mat.m._14;
				m._21 = mat.m._21; m._22 = mat.m._22; m._23 = mat.m._23; m._24 = mat.m._24;
				m._31 = mat.m._31; m._32 = mat.m._32; m._33 = mat.m._33; m._34 = mat.m._34;
				m._41 = mat.m._41; m._42 = mat.m._42; m._43 = mat.m._43; m._44 = mat.m._44;
				return *this;
			}
		};
	}

	using Int4x4 = nox::detail::Point4x4<s32>;
	using UInt4x4 = nox::detail::Point4x4<u32>;
	using Float4x4 = nox::detail::Point4x4<f32>;
	using Double4x4 = nox::detail::Point4x4<f64>;
}
#pragma warning(pop)