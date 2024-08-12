//	Copyright (C) 2024 NOX ENGINE All Rights Rserved.

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