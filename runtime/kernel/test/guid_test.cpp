//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	guid_test.cpp
///	@brief	Guid のバイト列と文字列の形が、Editor の System.Guid と一致することを確かめる
///	@details	同じ値を Editor/Studio.Wpf.UITests/GuidFormatTests.cs でも確かめている。値を変えるときは両方を直す

#include "pch.h"
#include "../basic_definition.h"
#include "../guid.h"

namespace
{
	struct GuidVector
	{
		std::array<nox::uint8, 16> bytes;	///< System.Guid の ToByteArray() の並び
		std::u8string_view hyphenated;		///< System.Guid の ToString("D")
		std::u8string_view compact;			///< System.Guid の ToString("N")
	};

	constexpr GuidVector kVectors[] =
	{
		//	data1 / data2 / data3 のバイトの並び (リトルエンディアン) を取り違えると、文字列の前半が入れ替わる値
		{
			{ 0x33, 0x22, 0x11, 0x00, 0x55, 0x44, 0x77, 0x66, 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff },
			u8"00112233-4455-6677-8899-aabbccddeeff",
			u8"00112233445566778899aabbccddeeff",
		},
		{
			{ 0x64, 0x5f, 0xa8, 0x3f, 0x17, 0x57, 0x62, 0x45, 0xb3, 0xfc, 0x2c, 0x96, 0x3f, 0x66, 0xaf, 0xa6 },
			u8"3fa85f64-5717-4562-b3fc-2c963f66afa6",
			u8"3fa85f6457174562b3fc2c963f66afa6",
		},
		{
			{},
			u8"00000000-0000-0000-0000-000000000000",
			u8"00000000000000000000000000000000",
		},
	};

	//	gtest は char8_t の文字列を表示できないので、比較に失敗したときの表示用に変える
	std::string ToNarrow(std::u8string_view text)
	{
		return std::string(reinterpret_cast<const char*>(text.data()), text.size());
	}

	//	文字列にする処理は constexpr で使える
	static_assert([]
		{
			std::array<nox::char8, 32> dest{};
			return nox::Guid::FromBytes(kVectors[1].bytes).ToStringCompact(dest) == kVectors[1].compact;
		}());
}

TEST(GuidTest, FormatsMatchEditorSystemGuid)
{
	for (const GuidVector& vector : kVectors)
	{
		const nox::Guid guid = nox::Guid::FromBytes(vector.bytes);

		std::array<nox::char8, 36> hyphenated{};
		EXPECT_EQ(ToNarrow(guid.ToString(hyphenated)), ToNarrow(vector.hyphenated));

		std::array<nox::char8, 32> compact{};
		EXPECT_EQ(ToNarrow(guid.ToStringCompact(compact)), ToNarrow(vector.compact));

		std::array<nox::uint8, 16> bytes{};
		guid.ToBytes(bytes);
		EXPECT_EQ(bytes, vector.bytes);
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
}
#endif // NOX_WINDOWS
