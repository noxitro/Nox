//	Copyright (C) 2026 NOX ENGINE All rights reserved.

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