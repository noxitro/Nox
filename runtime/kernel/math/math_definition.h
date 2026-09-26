//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	math_definition.h
///	@brief	math_definition
#pragma once
#include	<numbers>
#include	<concepts>

namespace nox::math
{
	/// @brief 円周率
	/// @tparam T 型
	template<std::floating_point T>
	constexpr	T	PI = std::numbers::pi_v<T>;


}