//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	string_util.h
///	@brief	string_util
#pragma once
#include	<algorithm>
#include	<ranges>
#include	<span>
#include	<string_view>
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
		//	収まらない分は切り詰める
		const size_t copy_size = std::min(source.size(), dest_buffer.size());
		std::ranges::copy(source.substr(0, copy_size), dest_buffer.data());

		//	終端文字を格納
		//	サイズがちょうどの場合は何もしない
		if (copy_size < dest_buffer.size())
		{
			dest_buffer[copy_size] = 0;
		}
	}
#pragma endregion
}