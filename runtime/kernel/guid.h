//	Copyright (C) 2025 NOX ENGINE All Rights Rserved.

///	@file	guid.h
///	@brief	guid
#pragma once
#include "basic_type.h"
#include "nox_string.h"
#include "type_traits/concepts.h"
#include "type_traits/type_traits.h"

namespace nox
{
	struct Guid
	{
	public:
		// 生成・初期化
		inline consteval Guid() noexcept :
			data1(0), data2(0), data3(0), data4{ 0 } 
		{
		}

		inline constexpr Guid(const Guid&) noexcept = default;
		inline constexpr Guid(Guid&&) noexcept = default;
		inline constexpr Guid& operator=(const Guid&) noexcept = default;
		inline constexpr Guid& operator=(Guid&&) noexcept = default;

		inline constexpr bool operator==(const Guid&) const noexcept = default;

		static inline constexpr  nox::Guid Empty() noexcept { return {}; }

		static Guid NewGuid() noexcept;

		inline constexpr std::u8string_view ToStringU8(std::span<nox::char8> dest)const noexcept
		{
			auto sv = ToStringImpl<nox::char8>(dest);
			return std::u8string_view{ sv.data(), sv.size() };
		}

		template<nox::concepts::Char T>
		inline constexpr nox::BasicString<T> ToString()const
		{
			nox::BasicString<T> dest;
			dest.resize(36);

			return this->ToStringImpl<T>(std::span<T>{ dest });
		}

		template<nox::concepts::Char T>
		inline constexpr std::basic_string_view<T> ToString(std::array<T, 36>& dest)const noexcept
		{
			return this->ToStringImpl<T>(std::span<T>{ dest });
		}
	private:
		template<class T>
		static constexpr T hex_nibble(nox::uint8 n)noexcept
		{
			return static_cast<T>(n < 10 ? (0x30u + n) : (0x61u + (n - 10))); // '0' or 'a'
		};

		template<nox::concepts::Char T>
		inline constexpr std::basic_string_view<T> ToStringImpl(std::span<T> dest)const noexcept
		{
			// "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx" = 36 chars
			constexpr std::size_t kLen = 36;
			if (dest.size() < kLen) return {};

		
			auto write_dash = [&dest](std::size_t& p) constexpr noexcept {
				dest[p++] = static_cast<T>(0x2D); // '-'
				};
			auto write_byte = [&dest](nox::uint8 v, std::size_t& p) constexpr noexcept {
				dest[p++] = hex_nibble< nox::uint8>(static_cast<nox::uint8>((v >> 4) & 0x0F));
				dest[p++] = hex_nibble< nox::uint8>(static_cast<nox::uint8>( v       & 0x0F));
				};

			std::size_t p = 0;

			// data1 (uint32) big-endian
			write_byte(static_cast<nox::uint8>((data1 >> 24) & 0xFFu), p);
			write_byte(static_cast<nox::uint8>((data1 >> 16) & 0xFFu), p);
			write_byte(static_cast<nox::uint8>((data1 >>  8) & 0xFFu), p);
			write_byte(static_cast<nox::uint8>((data1 >>  0) & 0xFFu), p);
			write_dash(p);

			// data2 (uint16) big-endian
			write_byte(static_cast<nox::uint8>((data2 >> 8) & 0xFFu), p);
			write_byte(static_cast<nox::uint8>((data2 >> 0) & 0xFFu), p);
			write_dash(p);

			// data3 (uint16) big-endian
			write_byte(static_cast<nox::uint8>((data3 >> 8) & 0xFFu), p);
			write_byte(static_cast<nox::uint8>((data3 >> 0) & 0xFFu), p);
			write_dash(p);

			// data4[0..1]
			write_byte(data4[0], p);
			write_byte(data4[1], p);
			write_dash(p);

			// data4[2..7]
			write_byte(data4[2], p);
			write_byte(data4[3], p);
			write_byte(data4[4], p);
			write_byte(data4[5], p);
			write_byte(data4[6], p);
			write_byte(data4[7], p);

			// 任意でヌル終端
			if (dest.size() > p) dest[p] = static_cast<T>(0);

			return std::basic_string_view<T>{ dest.data(), p };
		}
	private:
		// デフォルトメンバ初期化子で常に0初期化
		nox::uint32 data1;
		nox::uint16 data2;
		nox::uint16 data3;
		nox::uint8  data4[8];
	};
}