//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	string_util.h
///	@brief	string_util
#pragma once
#include	"advanced_type.h"
#include	"type_traits/type_traits.h"

namespace nox::util
{
#pragma region 文字列操作
	/// @brief 確保済みバッファに対しての文字列コピー
	/// @tparam CharType 
	/// @param source 
	/// @param dest_buffer 
	template <concepts::Char CharType>
	inline void StrCopy(std::basic_string_view<CharType> source, std::span<CharType> dest_buffer)
	{
	//	NOX_ASSERT(dest_buffer.size() >= source.size(), U"buffer size over");
		std::ranges::copy(source, dest_buffer.data());
		//	buffer[size] = std::char_traits<Char>::eof();	//	終端文字を格納

		//	終端文字を格納
		//	サイズがちょうどの場合は何もしない
		if (dest_buffer.size() < source.size())
		{
			dest_buffer[source.size()] = 0;
		}
	}
#pragma endregion
}