// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

///	@file	assertion.h
///	@brief	assertion
#pragma once
#include	<array>
#include	<cstddef>
#include	<source_location>
#include	<span>
#include	<string_view>
#include	"basic_definition.h"
#include	"basic_type.h"

#include	"type_traits/concepts.h"
#include	"type_traits/type_traits.h"

#include	"preprocessor/util.h"
#include	"string_format.h"
#include	"string_util.h"
#include	"assertion_id.h"
#include	"assertion_kernel.h"

namespace nox::detail
{
	/// @brief	エラー型 + 文面
	template<nox::assertion::AssertError Error, class... Args>
	inline void AssertFailed(const std::source_location& location, const Error& error, const nox::char8* format, const Args&... args)
	{
		//	動的メモリ確保を避けるため、失敗時のみスタックのバッファを使用
		std::array<nox::char8, 2048> buffer = { 0 };
		const std::span<nox::char8> whole(buffer);

		//	値が長くても、文面の領域は半分残す
		std::size_t offset = nox::detail::FormatDetail(error, whole.first(whole.size() / 2));
		if (offset > 0)
		{
			buffer[offset++] = u8'\n';
		}

		const std::span<nox::char8> rest = whole.subspan(offset);
		const std::span<nox::char8> message_area = rest.first(rest.size() - 1);

		if constexpr (sizeof...(Args) > 0)
		{
			nox::util::Format(message_area, format, args...);
		}
		else
		{
			nox::util::StrCopy(std::u8string_view{ format }, message_area);
		}

		nox::detail::Assert(Error::kId, buffer.data(), location);
	}

	/// @brief	文面 + 書式の引数 (LogicError 扱い)
	template<class Arg, class... Args>
	inline void AssertFailed(const std::source_location& location, const nox::char8* format, const Arg& arg, const Args&... args)
	{
		nox::detail::AssertFailed(location, nox::assertion::LogicError{}, format, arg, args...);
	}
}

#if NOX_DEBUG || NOX_RELEASE
/// @brief	アサート
/// @param	expression	条件式
/// @param	...			エラー型、書式、書式の引数。エラー型か書式の少なくとも一方が必須
#define	NOX_ASSERT(expression, ...) \
	((void)((!!(expression)) || (::nox::detail::AssertFailed(::std::source_location::current(), __VA_ARGS__), 0)))
//	end define

#else
#define	NOX_ASSERT(...)
#endif // NOX_DEBUG || NOX_RELEASE
