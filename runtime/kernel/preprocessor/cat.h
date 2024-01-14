//	Copyright (C) 2023 NOX ENGINE All Rights Rserved.

///	@file	cat.h
///	@brief	cat
#pragma once

///	文字列の連結
#define	NOX_PP_CAT(a, b) a##b
#define	NOX_PP_CAT_I(a, b) NOX_PP_CAT(a, b)

#if defined(__clang__)

#else
///	u8文字列に置き換え
#define	NOX_DETAIL_TO_U8STRING(x) NOX_PP_CAT(u8, x)
#endif // 0

