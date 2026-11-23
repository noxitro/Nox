//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	assertion_kernel.h
///	@brief	軽量なアサーションヘッダ
///			assertion.hのインクルードチェーンは複雑なため、kernelでは極力こちらを使用する
///			制約としてfmtフォーマット処理が使えません
#pragma once
#include	<array>
#include	<cstddef>
#include	<source_location>
#include	<span>
#include	<string_view>
#include	<type_traits>
#include	"basic_definition.h"
#include	"assertion_id.h"

namespace nox::detail
{
	/// @brief	アサート失敗を通知する
	/// @param	id				エラーID
	/// @param	message			表示する文面
	/// @param	source_location	発生箇所
	void	Assert(nox::assertion::ErrorId id, std::u8string_view message, const std::source_location& source_location);

	/// @brief	エラー型の値を整形する
	/// @return	書き込んだ文字数
	template<nox::assertion::AssertError Error>
	inline std::size_t FormatDetail(const Error& error, std::span<char8_t> buffer)
	{
		if constexpr (std::is_empty_v<Error>)
		{
			return 0;
		}
		else
		{
			error.Format(buffer);
			return std::u8string_view{ buffer.data() }.size();
		}
	}

	/// @brief	エラー型のみ
	template<nox::assertion::AssertError Error>
	inline void AssertFailed(const std::source_location& location, const Error& error)
	{
		//	動的メモリ確保を避けるため、失敗時のみスタックのバッファを使用
		std::array<char8_t, 1024> buffer = { 0 };
		nox::detail::FormatDetail(error, std::span<char8_t>(buffer).first(buffer.size() - 1));
		nox::detail::Assert(Error::kId, buffer.data(), location);
	}

	/// @brief	文面のみ (LogicError 扱い)
	inline void AssertFailed(const std::source_location& location, const char8_t* message)
	{
		nox::detail::Assert(nox::assertion::LogicError::kId, message, location);
	}
}

#if NOX_DEBUG || NOX_RELEASE
/// @brief	アサート
/// @param	expression	条件式
/// @param	...			エラー型か文面のどちらか1つ
#define	NOX_ASSERT_KERNEL(expression, ...) \
	((void)((!!(expression)) || (::nox::detail::AssertFailed(::std::source_location::current(), __VA_ARGS__), 0)))
//	end define

#else
#define	NOX_ASSERT_KERNEL(...)
#endif // NOX_DEBUG || NOX_RELEASE
