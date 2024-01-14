//	Copyright (C) 2023 NOX ENGINE All Rights Rserved.

///	@file	util.h
///	@brief	util
#pragma once
#include	"cat.h"

#if defined(__clang__)
/*
*@brief ユニークなローカル変数を定義します
*@note 関数内でのみ使用できます
*/
#define	NOX_LOCAL_SCOPE(Instance) static_assert(__PRETTY_FUNCTION__); const auto NOX_PP_CAT_I(__local, __LINE__) = Instance
#define	NOX_LOCAL_SCOPE_C(Instance) static_assert(__PRETTY_FUNCTION__); constexpr auto NOX_PP_CAT_I(__local, __LINE__) = Instance
#elif defined(_MSC_VER)
/*
*@brief ユニークなローカル変数を定義します
*@note 関数内でのみ使用できます
*/
#define	NOX_LOCAL_SCOPE(Instance) static_assert(__FUNCTION__); const auto NOX_PP_CAT_I(_local, __LINE__) = Instance
#define	NOX_LOCAL_SCOPE_C(Instance) static_assert(__FUNCTION__); constexpr auto NOX_PP_CAT_I(_local, __LINE__) = Instance

#endif