// Copyright (C) 2026 NOX ENGINE All rights reserved.

/// @file	matrix_util.h
/// @brief	matrix_util
#pragma once
#include	"matrix4d.h"

namespace nox::matrix
{
	template<nox::concepts::Arithmetic T>
	inline constexpr nox::detail::Matrix4D<T> MakeScaling(const nox::detail::Vector3D<T>& v)noexcept
	{
		return nox::detail::Matrix4D<T>{
				v.x, T{ 0 }, T{ 0 }, T{ 0 },
				T{ 0 }, v.y, T{ 0 }, T{ 0 },
				T{ 0 }, T{ 0 }, v.z, T{ 0 },
				T{ 0 }, T{ 0 }, T{ 0 }, T{1}
		};
	}


}