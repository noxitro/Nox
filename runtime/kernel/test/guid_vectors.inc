//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

//	GUID の値の表。Runtime の kernel_test (guid_test.cpp) と Editor のテスト (Studio.Wpf.UITests/GuidFormatTests.cs) が、
//	このファイルを読んで同じ値を確かめる。値を足すときも変えるときも、このファイルだけを直す。
//
//	NOX_GUID_VECTOR(バイト列, "D" の形, "N" の形)
//		バイト列: System.Guid の ToByteArray() の並び (data1 / data2 / data3 はリトルエンディアン) を 16 進 32 文字で
//		保存データと Editor との通信には "D" の形 (ハイフン付きの小文字 36 文字) を書く
//	NOX_GUID_INVALID(文字列)
//		読むときに受け付けない文字列 (Runtime の Guid::TryParse と、System.Guid の TryParseExact の "D" / "N" の両方)
//
//	C# 側は正規表現でこの 2 つのマクロの行だけを読むので、1 行に 1 つ書き、文字列の中に " を入れない。

//	data1 / data2 / data3 のバイトの並びを取り違えると、文字列の前半が入れ替わる値
NOX_GUID_VECTOR("33221100554477668899aabbccddeeff", "00112233-4455-6677-8899-aabbccddeeff", "00112233445566778899aabbccddeeff")
NOX_GUID_VECTOR("645fa83f17576245b3fc2c963f66afa6", "3fa85f64-5717-4562-b3fc-2c963f66afa6", "3fa85f6457174562b3fc2c963f66afa6")
NOX_GUID_VECTOR("00000000000000000000000000000000", "00000000-0000-0000-0000-000000000000", "00000000000000000000000000000000")
NOX_GUID_VECTOR("ffffffffffffffffffffffffffffffff", "ffffffff-ffff-ffff-ffff-ffffffffffff", "ffffffffffffffffffffffffffffffff")

NOX_GUID_INVALID("")
NOX_GUID_INVALID("3fa85f64-5717-4562-b3fc-2c963f66afa")
NOX_GUID_INVALID("3fa85f64-5717-4562-b3fc-2c963f66afa6a")
NOX_GUID_INVALID("3fa85f6457174562b3fc2c963f66afa")
NOX_GUID_INVALID("3fa85f6457174562b3fc2c963f66afa6a")
NOX_GUID_INVALID("3fa85f64x5717-4562-b3fc-2c963f66afa6")
NOX_GUID_INVALID("3fa85f6-45717-4562-b3fc-2c963f66afa6")
NOX_GUID_INVALID("3fa85f64-5717-4562-b3fc-2c963f66afg6")
NOX_GUID_INVALID("3fa85f6457174562b3fc2c963f66afg6")
NOX_GUID_INVALID("{3fa85f64-5717-4562-b3fc-2c963f66afa6}")
NOX_GUID_INVALID("3fa85f645717-4562-b3fc-2c963f66afa6-")
