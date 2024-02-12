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
#define	NOX_WIN64 1
#define	NOX_WINDOWS 1
#else
///	@brief X64環境か
#define	NOX_WIN64 0
#define	NOX_WINDOWS 0
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


#if !defined(_MSC_VER) && !defined(__clang__)
static_assert(false, "not support compiler");
#endif // defined()


#if NOX_DEBUG || NOX_RELEASE
#define	NOX_CONDITINAL_DEVELOP(x) x
#else
#define	NOX_CONDITINAL_DEVELOP(x)
#endif // NOX_DEBUG

