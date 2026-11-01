//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	assertion_id.h
///	@brief	assertion_id
#pragma once
#include	<concepts>
#include	<cstddef>
#include	<span>
#include	<string_view>
#include	<type_traits>

namespace nox::util
{
	/// @brief		定数評価中に失敗した場合にコンパイルエラーを発生させる
	/// @details	実際に呼び出されることはない
	[[noreturn]] consteval void ConstevalFail(const char* message)noexcept
	{
		// メッセージがログに出るので便利
		throw message;
	}
}

namespace nox::assertion
{
	/// @brief		アサーションID
	/// @details	ログの分類に使うASCII文字列
	struct ErrorId final
	{
		std::u8string_view value;

		consteval explicit ErrorId(std::u8string_view v)noexcept : value(v)
		{
			for (const char8_t c : v)
			{
				if (c > 0x7F)
				{
					nox::util::ConstevalFail("ErrorId must be ASCII");
				}
			}
		}
	};

	/// @brief		アサーションのエラー型
	/// @details	kIdが必須。メンバを持つ型はFormatも必須。
	///				アロケータの内部からも発火するので、文字列を所有する型は渡せない。
	template<class T>
	concept AssertError = std::is_trivially_destructible_v<T>
		&& std::same_as<std::remove_const_t<decltype(T::kId)>, ErrorId>
		&& (std::is_empty_v<T>
			|| requires(const T& error, std::span<char8_t> buffer) { error.Format(buffer); });

	/// @brief アサーションのデフォルト
	struct LogicError final
	{
		static constexpr ErrorId kId{ u8"LogicError" };
	};

	struct NotImplemented final
	{
		static constexpr ErrorId kId{ u8"NotImplemented" };
	};

	struct OutOfRange final
	{
		static constexpr ErrorId kId{ u8"OutOfRange" };

		constexpr explicit OutOfRange(std::size_t index, std::size_t range)noexcept :
			index(index),
			range(range)
		{
		}

		void Format(std::span<char8_t> dest)const;

	private:
		std::size_t index;
		std::size_t range;
	};
}
