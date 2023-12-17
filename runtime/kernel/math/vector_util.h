///	@file	vector_util.h
///	@brief	vector_util
#pragma once

#include	"vector4d.h"

namespace nox::vector
{
#pragma region 定数
	namespace detail
	{
		template<std::floating_point T>
		constexpr	nox::detail::Vector3D<T> Vector3DZero = nox::detail::Vector3D<T>(.0f, .0f, .0f);
	}

	constexpr	Vec3	Vec3Zero = vector::detail::Vector3DZero<f32>;
#pragma endregion

}