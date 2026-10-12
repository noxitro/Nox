//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	nameof_value.h
///	@brief	nameof_value
#pragma once
#include	<string_view>

namespace nox::util
{
	namespace detail
	{
		template<auto V>
		consteval auto GetValueName()noexcept
		{
#if defined(__clang__) || defined(__GNUC__)
			return __PRETTY_FUNCTION__;
#else
			return __FUNCSIG__;
#endif
		}
	}

	template<auto V>
	consteval auto NameofValue()noexcept
	{
		return nox::util::detail::GetValueName<V>();
	}
}