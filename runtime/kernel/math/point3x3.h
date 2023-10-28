///	@file	point3x3.h
///	@brief	point3x3
#pragma once
#include	"point3d.h"
#pragma warning(push)
#pragma warning(disable:4201)
namespace nox
{
	namespace detail
	{
		template<concepts::Arithmetic _ValueType>
		struct Point3x3
		{
			union
			{
				struct {
					_ValueType _11, _12, _13;
					_ValueType _21, _22, _23;
					_ValueType _31, _32, _33;
				}m;
				std::array<Point3D<_ValueType>, 3> v;
			};

			[[nodiscard]] constexpr Point3x3()noexcept :
				m{	1.0f, .0f, .0f,
					.0f, 1.0f, .0f,
					.0f, .0f, 1.0f } {}


			[[nodiscard]]	constexpr explicit Point3x3(
				_ValueType m00, _ValueType m01, _ValueType m02,
				_ValueType m10, _ValueType m11, _ValueType m12,
				_ValueType m20, _ValueType m21, _ValueType m22)noexcept :
				m{ m00, m01, m02,
					m10, m11, m12,
					m20, m21, m22 } {}

			//------------------------------------------------------
			//	operator
			//------------------------------------------------------
			[[nodiscard]]	inline constexpr bool operator==(const Point3x3&)const noexcept = delete;
			[[nodiscard]]	inline constexpr bool operator!=(const Point3x3&)const noexcept = delete;
		};
	}

	using Int3x3 = nox::detail::Point3x3<f32>;
	using UInt3x3 = nox::detail::Point3x3<u32>;
	using Float3x3 = nox::detail::Point3x3<f32>;
	using Double3x3 = nox::detail::Point3x3<f64>;
}
#pragma warning(pop)