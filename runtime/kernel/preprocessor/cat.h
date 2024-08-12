//	Copyright (C) 2023 NOX ENGINE All Rights Rserved.

///	@file	cat.h
///	@brief	cat
#pragma once

///	文字列の連結
#define	NOX_PP_CAT(a, b) a##b
#define	NOX_PP_CAT_I(a, b) NOX_PP_CAT(a, b)

///@brief	文字列化
#define NOX_PP_TO_STRING(x) #x
#define NOX_PP_TO_STRING_I(x) NOX_PP_TO_STRING(x)
#define NOX_PP_TO_STRING_U32(x) NOX_PP_CAT(U, #x)

#if defined(__clang__)

#else
///	u8文字列に置き換え
#define	NOX_DETAIL_TO_U8STRING(x) NOX_PP_CAT(u8, x)
#endif // 0

