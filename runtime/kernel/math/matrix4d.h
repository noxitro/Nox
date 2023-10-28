///	@file	matrix4d.h
///	@brief	matrix4d
#pragma once

#include	"vector4d.h"

namespace nox
{
	namespace detail
	{
		template<concepts::Arithmetic _ValueType>
		struct Matrix4D
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
				std::array<Vector4D<_ValueType>, 4U> v;
			};

			[[nodiscard]]	inline	constexpr Matrix4D()noexcept :
				m{ 1.0f, .0f, .0f, .0f,
					.0f, 1.0f, .0f, .0f,
					.0f, .0f, 1.0f, .0f,
					.0f, .0f, .0f, 1.0f } {}

			[[nodiscard]]	inline	constexpr explicit Matrix4D(
				_ValueType m00, _ValueType m01, _ValueType m02, _ValueType m03,
				_ValueType m10, _ValueType m11, _ValueType m12, _ValueType m13,
				_ValueType m20, _ValueType m21, _ValueType m22, _ValueType m23,
				_ValueType m30, _ValueType m31, _ValueType m32, _ValueType m33)noexcept :
				m{ m00,  m01, m02, m03,
					m10, m11, m12, m13,
					m20, m21, m22, m23,
					m30, m31, m32, m33 } {}

			//	オペレータ
			constexpr Matrix4D& operator =(const Matrix4D& mat) noexcept {
				m._11 = mat.m._11; m._12 = mat.m._12; m._13 = mat.m._13; m._14 = mat.m._14;
				m._21 = mat.m._21; m._22 = mat.m._22; m._23 = mat.m._23; m._24 = mat.m._24;
				m._31 = mat.m._31; m._32 = mat.m._32; m._33 = mat.m._33; m._34 = mat.m._34;
				m._41 = mat.m._41; m._42 = mat.m._42; m._43 = mat.m._43; m._44 = mat.m._44;
				return *this;
			}

			[[nodiscard]] constexpr bool operator ==(const Matrix4D& mat)const noexcept {
				return
					m._11 == mat.m._11 && m._12 == mat.m._12 && m._13 == mat.m._13 && m._14 == mat.m._14 &&
					m._21 == mat.m._21 && m._22 == mat.m._22 && m._23 == mat.m._23 && m._24 == mat.m._24 &&
					m._31 == mat.m._31 && m._32 == mat.m._32 && m._33 == mat.m._33 && m._34 == mat.m._34 &&
					m._41 == mat.m._41 && m._42 == mat.m._42 && m._43 == mat.m._43 && m._44 == mat.m._44;
			}

			[[nodiscard]]	constexpr Matrix4D& operator *= (const _ValueType s)noexcept {
				m._11 *= s; m._12 *= s; m._13 *= s; m._14 *= s;
				m._21 *= s; m._22 *= s; m._23 *= s; m._24 *= s;
				m._31 *= s; m._32 *= s; m._33 *= s; m._34 *= s;
				m._41 *= s; m._42 *= s; m._43 *= s; m._44 *= s;
				return *this;
			}
			[[nodiscard]]	constexpr Matrix4D operator * (const _ValueType s)const noexcept {
				return Matrix4D(
					m._11 * s, m._12 * s, m._13 * s, m._14 * s,
					m._21 * s, m._22 * s, m._23 * s, m._24 * s,
					m._31 * s, m._32 * s, m._33 * s, m._34 * s,
					m._41 * s, m._42 * s, m._43 * s, m._44 * s
				);
			}

			[[nodiscard]]	inline	constexpr Matrix4D& operator *= (const Matrix4D& mat)noexcept {
				//	1
				m._11 *= mat.m._11; m._12 *= mat.m._21; m._13 *= mat.m._31; m._14 *= mat.m._41;
				m._11 *= mat.m._12; m._12 *= mat.m._22; m._13 *= mat.m._32; m._14 *= mat.m._42;
				m._11 *= mat.m._13; m._12 *= mat.m._23; m._13 *= mat.m._33; m._14 *= mat.m._43;
				m._11 *= mat.m._14; m._12 *= mat.m._24; m._13 *= mat.m._34; m._14 *= mat.m._44;
				//	2
				m._21 *= mat.m._11; m._22 *= mat.m._21; m._23 *= mat.m._31; m._24 *= mat.m._41;
				m._21 *= mat.m._12; m._22 *= mat.m._22; m._23 *= mat.m._32; m._24 *= mat.m._42;
				m._21 *= mat.m._13; m._22 *= mat.m._23; m._23 *= mat.m._33; m._24 *= mat.m._43;
				m._21 *= mat.m._14; m._22 *= mat.m._24; m._23 *= mat.m._34; m._24 *= mat.m._44;
				//	3
				m._31 *= mat.m._11; m._32 *= mat.m._21; m._33 *= mat.m._31; m._34 *= mat.m._41;
				m._31 *= mat.m._12; m._32 *= mat.m._22; m._33 *= mat.m._32; m._34 *= mat.m._42;
				m._31 *= mat.m._13; m._32 *= mat.m._23; m._33 *= mat.m._33; m._34 *= mat.m._43;
				m._31 *= mat.m._14; m._32 *= mat.m._24; m._33 *= mat.m._34; m._34 *= mat.m._44;
				//	4
				m._41 *= mat.m._11; m._42 *= mat.m._21; m._43 *= mat.m._31; m._44 *= mat.m._41;
				m._41 *= mat.m._12; m._42 *= mat.m._22; m._43 *= mat.m._32; m._44 *= mat.m._42;
				m._41 *= mat.m._13; m._42 *= mat.m._23; m._43 *= mat.m._33; m._44 *= mat.m._43;
				m._41 *= mat.m._14; m._42 *= mat.m._24; m._43 *= mat.m._34; m._44 *= mat.m._44;
				return *this;
			}

			[[nodiscard]]	inline constexpr Matrix4D  operator * (const Matrix4D& mat) const noexcept {
				return Matrix4D(
					//	1
					m._11 * mat.m._11 + m._12 * mat.m._21 + m._13 * mat.m._31 + m._14 * mat.m._41,
					m._11 * mat.m._12 + m._12 * mat.m._22 + m._13 * mat.m._32 + m._14 * mat.m._42,
					m._11 * mat.m._13 + m._12 * mat.m._23 + m._13 * mat.m._33 + m._14 * mat.m._43,
					m._11 * mat.m._14 + m._12 * mat.m._24 + m._13 * mat.m._34 + m._14 * mat.m._44,
					//	2
					m._21 * mat.m._11 + m._22 * mat.m._21 + m._23 * mat.m._31 + m._24 * mat.m._41,
					m._21 * mat.m._12 + m._22 * mat.m._22 + m._23 * mat.m._32 + m._24 * mat.m._42,
					m._21 * mat.m._13 + m._22 * mat.m._23 + m._23 * mat.m._33 + m._24 * mat.m._43,
					m._21 * mat.m._14 + m._22 * mat.m._24 + m._23 * mat.m._34 + m._24 * mat.m._44,
					//	3
					m._31 * mat.m._11 + m._32 * mat.m._21 + m._33 * mat.m._31 + m._34 * mat.m._41,
					m._31 * mat.m._12 + m._32 * mat.m._22 + m._33 * mat.m._32 + m._34 * mat.m._42,
					m._31 * mat.m._13 + m._32 * mat.m._23 + m._33 * mat.m._33 + m._34 * mat.m._43,
					m._31 * mat.m._14 + m._32 * mat.m._24 + m._33 * mat.m._34 + m._34 * mat.m._44,
					//	4
					m._41 * mat.m._11 + m._42 * mat.m._21 + m._43 * mat.m._31 + m._44 * mat.m._41,
					m._41 * mat.m._12 + m._42 * mat.m._22 + m._43 * mat.m._32 + m._44 * mat.m._42,
					m._41 * mat.m._13 + m._42 * mat.m._23 + m._43 * mat.m._33 + m._44 * mat.m._43,
					m._41 * mat.m._14 + m._42 * mat.m._24 + m._43 * mat.m._34 + m._44 * mat.m._44
				);
			}


		};
	}

	using Mat4 = nox::detail::Matrix4D<f32>;
}

