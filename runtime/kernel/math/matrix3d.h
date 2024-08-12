///	@file	matrix3d.h
///	@brief	matrix3d
#pragma once
#include	"../basic_type.h"
#include	"../type_traits/concepts.h"

namespace nox
{
	namespace detail
	{
		template<concepts::Arithmetic _ValueType>
		struct Matrix3D
		{
			_ValueType _11, _12, _13;
			_ValueType _21, _22, _23;
			_ValueType _31, _32, _33;

			[[nodiscard]]	constexpr Matrix3D()noexcept :
				_11(1.0f), _12(.0f), _13(.0f),
				_21(.0f), _22(1.0f), _23(.0f),
				_31(.0f), _32(.0f), _33(1.0f) {}

			[[nodiscard]]	constexpr explicit Matrix3D(
				_ValueType m00, _ValueType m01, _ValueType m02,
				_ValueType m10, _ValueType m11, _ValueType m12,
				_ValueType m20, _ValueType m21, _ValueType m22)noexcept :
				_11(m00), _12(m01), _13(m02),
				_21(m10), _22(m11), _23(m12),
				_31(m20), _32(m21), _33(m22) {}

			[[nodiscard]]	constexpr Matrix3D(const Matrix3D& mat)noexcept :
				_11(mat._11), _12(mat._12), _13(mat._13),
				_21(mat._21), _22(mat._22), _23(mat._23),
				_31(mat._31), _32(mat._32), _33(mat._33) {}

			//	オペレータ
			constexpr Matrix3D& operator =(const Matrix3D& mat) {
				_11 = mat._11; _12 = mat._12; _13 = mat._13;
				_21 = mat._21; _22 = mat._22; _23 = mat._23;
				_31 = mat._31; _32 = mat._32; _33 = mat._33;
				return *this;
			}

			constexpr Matrix3D& operator *= (const _ValueType s) {
				_11 *= s; _12 *= s; _13 *= s;
				_21 *= s; _22 *= s; _23 *= s;
				_31 *= s; _32 *= s; _33 *= s;
				return *this;
			}
			constexpr Matrix3D operator * (const _ValueType s)const {
				return Matrix3D(
					_11 * s, _12 * s, _13 * s,
					_21 * s, _22 * s, _23 * s,
					_31 * s, _32 * s, _33 * s
				);
			}

			constexpr Matrix3D& operator *= (const Matrix3D& mat) {
				//	1
				_11 *= mat._11; _12 *= mat._21; _13 *= mat._31;
				_11 *= mat._12; _12 *= mat._22; _13 *= mat._32;
				_11 *= mat._13; _12 *= mat._23; _13 *= mat._33;
				//	2
				_21 *= mat._11; _22 *= mat._21; _23 *= mat._31;
				_21 *= mat._12; _22 *= mat._22; _23 *= mat._32;
				_21 *= mat._13; _22 *= mat._23; _23 *= mat._33;
				//	3
				_31 *= mat._11; _32 *= mat._21; _33 *= mat._31;
				_31 *= mat._12; _32 *= mat._22; _33 *= mat._32;
				_31 *= mat._13; _32 *= mat._23; _33 *= mat._33;
				return *this;
			}

			constexpr Matrix3D  operator * (const Matrix3D& mat) const {
				return Matrix3D(
					//	1
					_11 * mat._11 + _12 * mat._21 + _13 * mat._31,
					_11 * mat._12 + _12 * mat._22 + _13 * mat._32,
					_11 * mat._13 + _12 * mat._23 + _13 * mat._33,
					//	2
					_21 * mat._11 + _22 * mat._21 + _23 * mat._31,
					_21 * mat._12 + _22 * mat._22 + _23 * mat._32,
					_21 * mat._13 + _22 * mat._23 + _23 * mat._33,
					//	3
					_31 * mat._11 + _32 * mat._21 + _33 * mat._31,
					_31 * mat._12 + _32 * mat._22 + _33 * mat._32,
					_31 * mat._13 + _32 * mat._23 + _33 * mat._33
				);
			}
		};
	}

	using Mat3 = nox::detail::Matrix3D<nox::float_t>;
}