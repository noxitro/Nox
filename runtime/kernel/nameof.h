//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	nameof.h
///	@brief	nameof
#pragma once
#include	"type_traits/type_name.h"

#define NOX_U8_NAMEOF_TYPE(type) \
	[]<typename = type>()constexpr noexcept -> decltype(auto) { \
		return NOX_PP_CAT_I(u8, #type);\
	}()

#define NOX_U8_NAMEOF_FUNCTION(func) \
	[]<typename T = decltype(func)>()constexpr noexcept -> decltype(auto) { \
		return NOX_PP_CAT_I(u8, #func);\
	}()

#define NOX_U8_NAMEOF_VARIABLE(var) \
	[]<typename T = decltype(var)>()constexpr noexcept -> decltype(auto) { \
		return NOX_PP_CAT_I(u8, #var);\
	}()