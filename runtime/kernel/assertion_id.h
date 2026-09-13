//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	assertion_id.h
///	@brief	assertion_id
#pragma once
#include	<string_view>

namespace nox::assertion
{
	namespace id
	{
		/// @brief ログID
		/// @details ログIDを定義する構造体を継承することで、ログIDを定義できます
		struct ErrorId
		{
		};

		/// @brief 無効なログID
		struct Invalid : ErrorId
		{
			inline constexpr std::u16string_view operator()() const noexcept { return u"Invalid"; }
		};

		struct NullAccess : ErrorId
		{
			inline constexpr std::u16string_view operator()() const noexcept { return u"NullAccess"; }
		};

		struct OutOfRange : ErrorId
		{
			inline constexpr std::u16string_view operator()() const noexcept { return u"OutOfRange"; }
		};
	}
}