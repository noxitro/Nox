//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	guid_test.cpp
///	@brief	Guid のバイト列と文字列の形が、Editor の System.Guid と一致することを確かめる
///	@details	値は guid_vectors.inc にあり、Editor/Studio.Wpf.UITests/GuidFormatTests.cs も同じファイルを読む

#include "pch.h"
#include "../basic_definition.h"
#include "../guid.h"

namespace
{
	struct GuidVector
	{
		std::string_view bytes;			///< System.Guid の ToByteArray() の並びを 16 進 32 文字で
		std::string_view hyphenated;	///< System.Guid の ToString("D")
		std::string_view compact;		///< System.Guid の ToString("N")
	};

	constexpr GuidVector kVectors[] =
	{
#define NOX_GUID_VECTOR(bytes, hyphenated, compact) GuidVector{ bytes, hyphenated, compact },
#define NOX_GUID_INVALID(text)
#include "guid_vectors.inc"
#undef NOX_GUID_VECTOR
#undef NOX_GUID_INVALID
	};

	constexpr std::string_view kInvalidTexts[] =
	{
#define NOX_GUID_VECTOR(bytes, hyphenated, compact)
#define NOX_GUID_INVALID(text) std::string_view{ text },
#include "guid_vectors.inc"
#undef NOX_GUID_VECTOR
#undef NOX_GUID_INVALID
	};

	constexpr nox::uint8 HexDigit(char c)noexcept
	{
		return static_cast<nox::uint8>(c <= '9' ? c - '0' : c - 'a' + 10);
	}

	constexpr std::array<nox::uint8, 16> ToByteArray(std::string_view hex)noexcept
	{
		std::array<nox::uint8, 16> bytes{};
		for (std::size_t i = 0; i < bytes.size(); ++i)
		{
			bytes[i] = static_cast<nox::uint8>((HexDigit(hex[i * 2]) << 4) | HexDigit(hex[i * 2 + 1]));
		}
		return bytes;
	}

	std::string ToUpper(std::string_view text)
	{
		std::string upper(text);
		for (char& c : upper)
		{
			if (c >= 'a' && c <= 'z')
			{
				c = static_cast<char>(c - 'a' + 'A');
			}
		}
		return upper;
	}

	//	文字列との変換は constexpr で使える
	static_assert([]
		{
			std::array<char, 36> dest{};
			const std::optional<nox::Guid> guid = nox::Guid::TryParse(kVectors[1].compact);
			return guid.has_value() && guid->ToString(dest) == kVectors[1].hyphenated;
		}());
	static_assert(!nox::Guid::TryParse(std::string_view{ "{3fa85f64-5717-4562-b3fc-2c963f66afa6}" }).has_value());
}

TEST(GuidTest, VectorsAreLoaded)
{
	//	マクロの書き間違いで 0 件になっても、他のテストは素通りしてしまうので件数を確かめる
	EXPECT_GE(std::size(kVectors), 3u);
	EXPECT_GE(std::size(kInvalidTexts), 5u);
}

TEST(GuidTest, FormatsMatchEditorSystemGuid)
{
	for (const GuidVector& vector : kVectors)
	{
		const std::array<nox::uint8, 16> expected_bytes = ToByteArray(vector.bytes);
		const nox::Guid guid = nox::Guid::FromBytes(expected_bytes);

		std::array<char, 36> hyphenated{};
		EXPECT_EQ(guid.ToString(hyphenated), vector.hyphenated);

		std::array<char, 32> compact{};
		EXPECT_EQ(guid.ToStringCompact(compact), vector.compact);

		std::array<nox::uint8, 16> bytes{};
		guid.ToBytes(bytes);
		EXPECT_EQ(bytes, expected_bytes) << vector.hyphenated;
	}
}

TEST(GuidTest, ParseAcceptsHyphenatedAndCompactInAnyCase)
{
	for (const GuidVector& vector : kVectors)
	{
		const nox::Guid expected = nox::Guid::FromBytes(ToByteArray(vector.bytes));
		const std::string upper_hyphenated = ToUpper(vector.hyphenated);
		const std::string upper_compact = ToUpper(vector.compact);

		for (const std::string_view text : { vector.hyphenated, vector.compact, std::string_view{ upper_hyphenated }, std::string_view{ upper_compact } })
		{
			const std::optional<nox::Guid> parsed = nox::Guid::TryParse(text);
			ASSERT_TRUE(parsed.has_value()) << text;
			EXPECT_EQ(*parsed, expected) << text;
		}

		//	char8_t の文字列でも読める
		const std::u8string u8_text(vector.hyphenated.begin(), vector.hyphenated.end());
		const std::optional<nox::Guid> parsed_u8 = nox::Guid::TryParse(std::u8string_view{ u8_text });
		ASSERT_TRUE(parsed_u8.has_value()) << vector.hyphenated;
		EXPECT_EQ(*parsed_u8, expected) << vector.hyphenated;
	}
}

TEST(GuidTest, ParseRejectsOtherForms)
{
	for (const std::string_view text : kInvalidTexts)
	{
		EXPECT_FALSE(nox::Guid::TryParse(text).has_value()) << "[" << text << "]";
	}
}

TEST(GuidTest, EmptyIsAllZero)
{
	std::array<nox::uint8, 16> bytes{};
	bytes.fill(0xff);
	nox::Guid::Empty().ToBytes(bytes);
	EXPECT_EQ(bytes, (std::array<nox::uint8, 16>{}));
	EXPECT_EQ(nox::Guid::FromBytes(bytes), nox::Guid::Empty());
}

#if NOX_WINDOWS
//	Editor の System.Guid.NewGuid() と同じく、乱数で作る version 4 の UUID になっていること
TEST(GuidTest, NewGuidIsRandomVersion4)
{
	const nox::Guid first = nox::Guid::NewGuid();
	const nox::Guid second = nox::Guid::NewGuid();
	EXPECT_NE(first, second);

	std::array<nox::uint8, 16> bytes{};
	first.ToBytes(bytes);
	EXPECT_EQ(bytes[7] >> 4, 4);			//	data3 の上位 4 bit (リトルエンディアンなので bytes[7])
	EXPECT_EQ(bytes[8] & 0xC0, 0x80);		//	data4[0] の上位 2 bit (variant)

	//	文字列にして読み直すと元に戻る
	std::array<char, 36> text{};
	const std::optional<nox::Guid> parsed = nox::Guid::TryParse(first.ToString(text));
	ASSERT_TRUE(parsed.has_value());
	EXPECT_EQ(*parsed, first);
}
#endif // NOX_WINDOWS
