//	Copyright (C) 2024 NOX ENGINE All Rights Rserved.

///	@file	nameof.h
///	@brief	nameof
#pragma once
#include	"type_traits/type_name.h"

namespace nox::detail
{
	template<class T>
	inline constexpr bool IsVariable(T&& val)noexcept
	{
		return true;
	}
}

///@brief	変数名を取得する
#define	NOX_NAMEOF_VARIABLE(val) \
	[]()constexpr noexcept{\
		static_assert(::nox::detail::IsVariable(val));\
		return #val;\
	}();
//end define