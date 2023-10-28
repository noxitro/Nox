///	@file	basic_definition.h
///	@brief	汎用的に使う定義など
#pragma once

#include	"../gsl/gsl"

namespace nox
{
	/// @brief not_null
	using gsl::not_null;
}

#if _WIN64
///	@brief X64環境か
#define	NOX_X64 1
#else
///	@brief X64環境か
#define	NOX_X64 0
#endif // _WIN64


#if _DEBUG
///	@brief	Debugビルドか
#define NOX_DEBUG 1
#else
/**
 * @brief	Debugビルドか
*/
#define NOX_DEBUG 0
#endif // _DEBUG

#if _MASTER
/**
 * @brief	Masterビルドか
*/
#define	NOX_MASTER 1
#else
/**
 * @brief	Masterビルドか
*/
#define	NOX_MASTER 0
#endif // _MASTER

#if !NOX_DEBUG && !NOX_MASTER
/**
 * @brief	Releaseビルドか
*/
#define	NOX_RELEASE 1
#else
/**
 * @brief	Releaseビルドか
*/
#define	NOX_RELEASE 0
#endif // NOX_DEBUG

#if NOX_DEBUG || NOX_RELEASE
/**
 * @brief	開発ビルドか
*/
#define	NOX_DEVELOP 1
#else
/**
 * @brief	開発ビルドか
*/
#define	NOX_DEVELOP 0
#endif // NOX_DEBUG || NOX_RELEASE

#define	NOX_PP(x) x

///	文字列の連結
#define	NOX_PP_CAT(a, b) a##b
#define	NOX_PP_CAT_I(a, b) NOX_PP_CAT(a, b)

#if defined(__clang__)

#else
///	u8文字列に置き換え
#define	NOX_DETAIL_TO_U8STRING(x) NOX_PP_CAT(u8, x)
#endif // 0

#if !defined(_MSC_VER) && !defined(__clang__)
static_assert(false, "not support compiler");
#endif // defined()

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


#if NOX_DEBUG || NOX_RELEASE
#define	NOX_CONDITINAL_DEBUG(x) x
#else
#define	NOX_CONDITINAL_DEBUG(x)
#endif // NOX_DEBUG

