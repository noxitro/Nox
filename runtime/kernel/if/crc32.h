///	@file	crc32.h
///	@brief	crc32
#pragma once
#include	"basic_type.h"
#include	<string_view>
#include	<numeric>

namespace nox::util
{
	/// @brief 文字列から一意な値を取得
	/// @param str 文字列
	/// @return 4byteの値
	inline	constexpr u32	crc32(const std::u8string_view str)noexcept
	{
		//			constexpr u32 CRC32POLY1 = 0x04C11DB7UL;
		constexpr u32 CRC32POLY2 = 0xEDB88320UL;/* 左右逆転 */

		u32 r = 0xFFFFFFFFUL;
		for (s32 i = 0; i < static_cast<s32>(str.length()); i++)
		{
			r ^= str.at(i);
			for (s32 j = 0; j < std::numeric_limits<u8>::digits; j++)
			{
				if (r & 1)
				{
					r = (r >> 1) ^ CRC32POLY2;
				}
				else
				{
					r >>= 1;
				}
			}
		}
		return r ^ 0xFFFFFFFFUL;
	}

#if defined(__clang__)
	inline	constexpr u32	crc32(const std::string_view str)noexcept
	{
		//			constexpr u32 CRC32POLY1 = 0x04C11DB7UL;
		constexpr u32 CRC32POLY2 = 0xEDB88320UL;/* 左右逆転 */

		u32 r = 0xFFFFFFFFUL;
		for (s32 i = 0; i < static_cast<s32>(str.length()); i++)
		{
			r ^= str.at(i);
			for (s32 j = 0; j < std::numeric_limits<u8>::digits; j++)
			{
				if (r & 1)
				{
					r = (r >> 1) ^ CRC32POLY2;
				}
				else
				{
					r >>= 1;
				}
			}
		}
		return r ^ 0xFFFFFFFFUL;
	}
#endif
}