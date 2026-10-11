// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

using Xunit;

namespace Studio.Wpf.UITests;

/// <summary>
/// System.Guid のバイト列と文字列の形が、Runtime の nox::Guid と一致することを確かめる。
/// </summary>
/// <remarks>
/// 同じ値を runtime/kernel/test/guid_test.cpp でも確かめている。値を変えるときは両方を直す。
/// メタファイルの GUID は ToString("N") で書いている (Core/AssetManager.cs)。
/// UI を動かさないテストだが、Editor の C# のテストのプロジェクトがここだけなので置いている。
/// </remarks>
public sealed class GuidFormatTests
{
	public static TheoryData<byte[], string, string> Vectors => new()
	{
		// data1 / data2 / data3 のバイトの並び (リトルエンディアン) を取り違えると、文字列の前半が入れ替わる値
		{
			new byte[] { 0x33, 0x22, 0x11, 0x00, 0x55, 0x44, 0x77, 0x66, 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff },
			"00112233-4455-6677-8899-aabbccddeeff",
			"00112233445566778899aabbccddeeff"
		},
		{
			new byte[] { 0x64, 0x5f, 0xa8, 0x3f, 0x17, 0x57, 0x62, 0x45, 0xb3, 0xfc, 0x2c, 0x96, 0x3f, 0x66, 0xaf, 0xa6 },
			"3fa85f64-5717-4562-b3fc-2c963f66afa6",
			"3fa85f6457174562b3fc2c963f66afa6"
		},
		{
			new byte[16],
			"00000000-0000-0000-0000-000000000000",
			"00000000000000000000000000000000"
		},
	};

	[Theory]
	[MemberData(nameof(Vectors))]
	public void FormatsMatchRuntimeNoxGuid(byte[] bytes, string hyphenated, string compact)
	{
		var guid = new Guid(bytes);
		Assert.Equal(hyphenated, guid.ToString("D"));
		Assert.Equal(compact, guid.ToString("N"));
		Assert.Equal(bytes, Guid.Parse(compact).ToByteArray());
		Assert.Equal(bytes, Guid.Parse(hyphenated).ToByteArray());
	}
}
