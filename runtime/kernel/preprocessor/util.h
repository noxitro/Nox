//	Copyright (C) 2023 NOX ENGINE All Rights Rserved.

///	@file	util.h
///	@brief	util
#pragma once
#include	"cat.h"

///@brief 受け取った引数をそのまま返す
#define	NOX_PP_IDENTITY(...) __VA_ARGS__

#if defined(__clang__)
/*
*@brief ユニークなローカル変数を定義します
*@note 関数内でのみ使用できます
*/
#define	NOX_LOCAL_SCOPE(x) static_assert(__PRETTY_FUNCTION__); const decltype(x) NOX_PP_CAT_I(__local, __LINE__){x}
#define	NOX_LOCAL_SCOPE_C(x) static_assert(__PRETTY_FUNCTION__); constexpr decltype(x) NOX_PP_CAT_I(__local, __LINE__){x}
#elif defined(_MSC_VER)
/*
*@brief ユニークなローカル変数を定義します
*@note 関数内でのみ使用できます
*/
#define	NOX_LOCAL_SCOPE(x) static_assert(__FUNCTION__); const decltype(x) NOX_PP_CAT_I(__local, __LINE__){x}
#define	NOX_LOCAL_SCOPE_C(x) static_assert(__FUNCTION__); constexpr decltype(x) NOX_PP_CAT_I(__local, __LINE__){x}

#endif

///@brief	引数の最後を取得
#define NOX_PP_GET_ARGUMENT_LAST(...) NOX_PP_GET_ARGUMENT_IMPL(NOX_PP_VA_LENGTH(__VA_ARGS__), _, __VA_ARGS__ ,,,,,,,,,,,)
#define NOX_PP_GET_ARGUMENT_IMPL(N, ...) NOX_PP_CAT_I(NOX_PP_GET_ARGUMENT_IMPL_, N)(__VA_ARGS__)
#define NOX_PP_GET_ARGUMENT_IMPL_0(_0, ...) _0
#define NOX_PP_GET_ARGUMENT_IMPL_1(_0, _1, ...) _1
#define NOX_PP_GET_ARGUMENT_IMPL_2(_0, _1, _2, ...) _2
#define NOX_PP_GET_ARGUMENT_IMPL_3(_0, _1, _2, _3, ...) _3
#define NOX_PP_GET_ARGUMENT_IMPL_4(_0, _1, _2, _3, _4, ...) _4
#define NOX_PP_GET_ARGUMENT_IMPL_5(_0, _1, _2, _3, _4, _5, ...) _5
#define NOX_PP_GET_ARGUMENT_IMPL_6(_0, _1, _2, _3, _4, _5, _6, ...) _6
#define NOX_PP_GET_ARGUMENT_IMPL_7(_0, _1, _2, _3, _4, _5, _6, _7, ...) _7
#define NOX_PP_GET_ARGUMENT_IMPL_8(_0, _1, _2, _3, _4, _5, _6, _7, _8, ...) _8
#define NOX_PP_GET_ARGUMENT_IMPL_9(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, ...) _9
#define NOX_PP_GET_ARGUMENT_IMPL_10(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, ...) _10
