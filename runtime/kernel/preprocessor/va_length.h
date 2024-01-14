//	Copyright (C) 2023 NOX ENGINE All Rights Rserved.

///	@file	va_length.h
///	@brief	va_length
#pragma once

///@brief	マクロの可変長引数の長さを取得
#define NOX_PP_VA_LENGTH(...) NOX_PP_VA_LENGTH_IMPL(0, ## __VA_ARGS__, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)

///@brief	マクロの可変長引数の長さを取得(実装部分)
#define NOX_PP_VA_LENGTH_IMPL(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, N, ...) N