//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	assertion_kernel.h
///	@brief	軽量なアサーションヘッダ
///			assertion.hのインクルードチェーンは複雑なため、kernelでは極力こちらを使用する
///			制約としてfmtフォーマット処理が使えません
#pragma once
#include	<source_location>
#include	"basic_definition.h"
#include	"assertion_id.h"

namespace nox::assertion::detail
{
	void AssertKernel(std::u16string_view error_category, std::u8string_view message, std::wstring_view filename, const std::source_location location);
	void AssertKernel(std::u16string_view error_category, std::u16string_view message, std::wstring_view filename, const std::source_location location);

	template<std::derived_from<nox::assertion::id::ErrorId> Id> //requires(std::is_invocable_r_v<std::u8string_view, Id>)
	inline void AssertKernel(std::u8string_view message, std::wstring_view filename, const std::source_location location)
	{
		nox::assertion::detail::AssertKernel(Id()(), message, filename, location);
	}

	template<std::derived_from<nox::assertion::id::ErrorId> Id> //requires(std::is_invocable_r_v<std::u8string_view, Id>)
	inline void AssertKernel(std::u16string_view message, std::wstring_view filename, const std::source_location location)
	{
		nox::assertion::detail::AssertKernel(Id()(), message, filename, location);
	}
}

#if NOX_DEBUG || NOX_RELEASE
#define NOX_ASSERT_KERNEL_ID_IMPL(expression, id, message) \
	((void)(			\
	(!!(expression)) || \
	(::nox::assertion::detail::AssertKernel<id>(message, __FILEW__, ::std::source_location::current()),0))\
	)
//	end define

/// @brief アサート
#define NOX_ASSERT_KERNEL_ID(expression, id, message) NOX_ASSERT_KERNEL_ID_IMPL(expression, id, message)
#define	NOX_ASSERT_KERNEL(expression, message) NOX_ASSERT_KERNEL_ID(expression, ::nox::assertion::id::Invalid, message)
#else
#define	NOX_ASSERT_KERNEL(...) 
#define NOX_ASSERT_KERNEL_ID(...)
#endif // NOX_DEBUG || NOX_RELEASE
