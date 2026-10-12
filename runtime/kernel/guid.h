//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	guid.h
///	@brief	guid
#pragma once
#include <array>
#include <optional>
#include <string_view>
#include "basic_type.h"
#include "nox_string.h"
#include "type_traits/concepts.h"
#include "type_traits/type_traits.h"

namespace nox
{
	struct Guid final
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

		/// @brief		16 byte の並びから作る
		/// @details	並びは Editor の System.Guid の ToByteArray() / Guid(byte[]) と同じにする。
		///				data1 / data2 / data3 はリトルエンディアン、data4 は先頭から順に並べる。
		///				一致は kernel_test の guid_test.cpp と Studio.Wpf.UITests の GuidFormatTests.cs で確かめている
		static inline constexpr nox::Guid FromBytes(std::span<const nox::uint8, 16> bytes)noexcept
		{
			nox::Guid guid;
			guid.data1 = static_cast<nox::uint32>(bytes[0])
				| (static_cast<nox::uint32>(bytes[1]) << 8)
				| (static_cast<nox::uint32>(bytes[2]) << 16)
				| (static_cast<nox::uint32>(bytes[3]) << 24);
			guid.data2 = static_cast<nox::uint16>(bytes[4] | (bytes[5] << 8));
			guid.data3 = static_cast<nox::uint16>(bytes[6] | (bytes[7] << 8));
			for (std::size_t i = 0; i < 8; ++i)
			{
				guid.data4[i] = bytes[8 + i];
			}
			return guid;
		}

		/// @brief 16 byte の並びに書き出す (並びは FromBytes と同じ)
		inline constexpr void ToBytes(std::span<nox::uint8, 16> dest)const noexcept
		{
			dest[0] = static_cast<nox::uint8>(data1 & 0xFFu);
			dest[1] = static_cast<nox::uint8>((data1 >> 8) & 0xFFu);
			dest[2] = static_cast<nox::uint8>((data1 >> 16) & 0xFFu);
			dest[3] = static_cast<nox::uint8>((data1 >> 24) & 0xFFu);
			dest[4] = static_cast<nox::uint8>(data2 & 0xFFu);
			dest[5] = static_cast<nox::uint8>((data2 >> 8) & 0xFFu);
			dest[6] = static_cast<nox::uint8>(data3 & 0xFFu);
			dest[7] = static_cast<nox::uint8>((data3 >> 8) & 0xFFu);
			for (std::size_t i = 0; i < 8; ++i)
			{
				dest[8 + i] = data4[i];
			}
		}

		/// @brief ハイフン付きの小文字 36 文字 ("00112233-4455-6677-8899-aabbccddeeff")。System.Guid の ToString("D") と同じ
		inline constexpr std::u8string_view ToStringU8(std::span<nox::char8> dest)const noexcept
		{
			auto sv = ToStringImpl<nox::char8>(dest, true);
			return std::u8string_view{ sv.data(), sv.size() };
		}

		/// @brief ハイフン付きの小文字 36 文字。System.Guid の ToString("D") と同じ
		template<nox::concepts::Char T>
		inline constexpr nox::StlBasicString<T> ToString()const
		{
			nox::StlBasicString<T> dest;
			dest.resize(36);

			this->ToStringImpl<T>(std::span<T>{ dest }, true);
			return dest;
		}

		/// @brief		ハイフン付きの小文字 36 文字。System.Guid の ToString("D") と同じ
		/// @details	保存データと Editor との通信には、この形を書く
		template<nox::concepts::Char T>
		inline constexpr std::basic_string_view<T> ToString(std::array<T, 36>& dest)const noexcept
		{
			return this->ToStringImpl<T>(std::span<T>{ dest }, true);
		}

		/// @brief		ハイフン無しの小文字 32 文字 ("00112233445566778899aabbccddeeff")。System.Guid の ToString("N") と同じ
		/// @details	保存データには ToString の形 ("D") を書く。この形は、今のメタファイルなど既存のデータとの互換のために残す
		template<nox::concepts::Char T>
		inline constexpr std::basic_string_view<T> ToStringCompact(std::array<T, 32>& dest)const noexcept
		{
			return this->ToStringImpl<T>(std::span<T>{ dest }, false);
		}

		/// @brief		文字列から作る
		/// @details	"D" (ハイフン付き 36 文字) と "N" (ハイフン無し 32 文字) を受け付け、英字の大文字と小文字は区別しない。
		///				System.Guid の TryParseExact(text, "D") / TryParseExact(text, "N") と同じものを受け付ける
		///				(波括弧や空白を含む形は受け付けない)。一致は guid_vectors.inc を両側のテストが読んで確かめている
		/// @return		形が正しくなければ std::nullopt
		template<nox::concepts::Char T>
		static inline constexpr std::optional<nox::Guid> TryParse(std::basic_string_view<T> text)noexcept
		{
			const bool is_hyphenated = text.size() == 36;
			if (!is_hyphenated && text.size() != 32)
			{
				return std::nullopt;
			}

			//	文字列の並び (data1 / data2 / data3 は上位のバイトから) のまま 16 byte を読む
			std::array<nox::uint8, 16> digits{};
			std::size_t p = 0;
			for (std::size_t i = 0; i < digits.size(); ++i)
			{
				if (is_hyphenated && (i == 4 || i == 6 || i == 8 || i == 10))
				{
					if (text[p] != static_cast<T>(0x2D))	// '-'
					{
						return std::nullopt;
					}
					++p;
				}
				const int high = HexValue(text[p]);
				const int low = HexValue(text[p + 1]);
				if (high < 0 || low < 0)
				{
					return std::nullopt;
				}
				digits[i] = static_cast<nox::uint8>((high << 4) | low);
				p += 2;
			}

			nox::Guid guid;
			guid.data1 = (static_cast<nox::uint32>(digits[0]) << 24) | (static_cast<nox::uint32>(digits[1]) << 16)
				| (static_cast<nox::uint32>(digits[2]) << 8) | static_cast<nox::uint32>(digits[3]);
			guid.data2 = static_cast<nox::uint16>((digits[4] << 8) | digits[5]);
			guid.data3 = static_cast<nox::uint16>((digits[6] << 8) | digits[7]);
			for (std::size_t i = 0; i < 8; ++i)
			{
				guid.data4[i] = digits[8 + i];
			}
			return guid;
		}
	private:
		/// @brief 16 進の 1 文字の値。16 進でなければ -1
		template<nox::concepts::Char T>
		static constexpr int HexValue(T c)noexcept
		{
			if (c >= static_cast<T>(0x30) && c <= static_cast<T>(0x39))	// '0'-'9'
			{
				return static_cast<int>(c - static_cast<T>(0x30));
			}
			if (c >= static_cast<T>(0x61) && c <= static_cast<T>(0x66))	// 'a'-'f'
			{
				return static_cast<int>(c - static_cast<T>(0x61)) + 10;
			}
			if (c >= static_cast<T>(0x41) && c <= static_cast<T>(0x46))	// 'A'-'F'
			{
				return static_cast<int>(c - static_cast<T>(0x41)) + 10;
			}
			return -1;
		}

		template<class T>
		static constexpr T hex_nibble(nox::uint8 n)noexcept
		{
			return static_cast<T>(n < 10 ? (0x30u + n) : (0x61u + (n - 10))); // '0' or 'a'
		};

		/// @param is_hyphenated true なら "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx" (36 文字)、false ならハイフン無し (32 文字)
		template<nox::concepts::Char T>
		inline constexpr std::basic_string_view<T> ToStringImpl(std::span<T> dest, bool is_hyphenated)const noexcept
		{
			const std::size_t length = is_hyphenated ? 36 : 32;
			if (dest.size() < length) return {};

			auto write_dash = [&dest, is_hyphenated](std::size_t& p) constexpr noexcept {
				if (is_hyphenated)
				{
					dest[p++] = static_cast<T>(0x2D); // '-'
				}
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