//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	bit_flag.h
///	@brief	ビットフラグ操作クラス
#pragma once
#include    "type_traits/concepts.h"
#include    "basic_type.h"

namespace nox
{
    namespace detail
    {
        template<std::integral RawValueType, class T = nox::uint8>
        class BitFlagImpl
        {
        public:
            inline constexpr BitFlagImpl()noexcept : raw_value(0) {}

            inline constexpr void On(const T flag)noexcept
            {
                raw_value |= static_cast<std::underlying_type_t<T>>(flag);
            }

            inline constexpr void Off(const T flag)noexcept
            {
                raw_value &= ~static_cast<std::underlying_type_t<T>>(flag);
            }

            inline constexpr bool IsOn(const T flag)const noexcept
            {
                return (raw_value & static_cast<std::underlying_type_t<T>>(flag)) != 0;
            }

            inline constexpr T Get()const noexcept
            {
                return raw_value;
            }

        private:
            RawValueType raw_value;
        };
    }

	using BitFlag8 = detail::BitFlagImpl<nox::uint8>;
	using BitFlag16 = detail::BitFlagImpl<nox::uint16>;
	using BitFlag32 = detail::BitFlagImpl<nox::uint32>;
	using BitFlag64 = detail::BitFlagImpl<nox::uint64>;

	template<nox::concepts::Enum T>
	using BitFlag = detail::BitFlagImpl<std::underlying_type_t<T>, T>;
}