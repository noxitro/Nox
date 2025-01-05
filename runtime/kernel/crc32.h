///	@file	crc32.h
///	@brief	crc32
#pragma once
#include	<string_view>
#include	<numeric>
#include	<limits>

#include	"basic_type.h"

namespace nox::util
{
	template<typename T>
	inline	constexpr uint32	Crc32(const std::basic_string_view<T> str)noexcept
	{
		//			constexpr u32 CRC32POLY1 = 0x04C11DB7UL;
		constexpr uint32 CRC32POLY2 = 0xEDB88320UL;/* 左右逆転 */

		uint32 r = 0xFFFFFFFFUL;
		for (int32 i = 0; i < static_cast<int32>(str.length()); i++)
		{
			r ^= str.at(i);
			for (int32 j = 0; j < std::numeric_limits<T>::digits; j++)
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
}