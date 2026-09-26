//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	raw_keyboard_test.cpp
///	@brief	Raw Input / 従来のキーメッセージから HID のキーコードへの変換テスト
///	@details	実機の入力列は、開発ビルドの runtime.exe を --log-raw-keyboard 付きで起動し、
///				デバッグ出力に出る「RawKeyboard { MakeCode, Flags, VKey }」を
///				RawKeyboardRecord の配列へそのまま貼って追加する。

#include "pch.h"
#include "../raw_keyboard.h"

#include <span>
#include <vector>

namespace
{
	/// @brief RAWKEYBOARD から変換に使う 3 つの値
	struct RawKeyboardRecord
	{
		nox::uint16 make_code;
		nox::uint16 flags;
		nox::uint16 virtual_key;
	};

	constexpr nox::uint16 kBreak = nox::hid::kRawKeyBreak;
	constexpr nox::uint16 kE0 = nox::hid::kRawKeyE0;
	constexpr nox::uint16 kE1 = nox::hid::kRawKeyE1;

	/// @brief 入力列を順に変換し、送られる入力だけを返す
	std::vector<nox::hid::RawKeyboardInput> TranslateSequence(const std::span<const RawKeyboardRecord> records)
	{
		std::vector<nox::hid::RawKeyboardInput> inputs;
		nox::hid::RawKeyboardTranslateState state{};
		for (const RawKeyboardRecord& record : records)
		{
			nox::hid::RawKeyboardInput input{};
			if (nox::hid::TranslateRawKeyboardInput(
				record.make_code,
				record.flags,
				record.virtual_key,
				state,
				input) == true)
			{
				inputs.push_back(input);
			}
		}
		return inputs;
	}

	/// @brief 押下 1 件ぶんの Raw Input をキーコードへ引く
	nox::hid::KeyCode MapRaw(const nox::uint16 make_code, const nox::uint16 flags, const nox::uint16 virtual_key)
	{
		nox::hid::RawKeyboardTranslateState state{};
		nox::hid::RawKeyboardInput input{};
		if (nox::hid::TranslateRawKeyboardInput(make_code, flags, virtual_key, state, input) == false)
		{
			return nox::hid::KeyCode::Unknown;
		}
		return nox::hid::MapKeyCode(input);
	}
}

//	---------------------------------------------------------------------------
//	RAWKEYBOARD からの変換
//	---------------------------------------------------------------------------

TEST(RawKeyboardTranslateTest, KeyDownAndUp)
{
	constexpr RawKeyboardRecord kRecords[] =
	{
		{ 0x001E, 0x0000, 0x0041 },	//	A 押下
		{ 0x001E, kBreak, 0x0041 },	//	A 解放
	};
	const auto inputs = TranslateSequence(kRecords);
	ASSERT_EQ(inputs.size(), 2u);

	EXPECT_TRUE(inputs[0].is_down);
	EXPECT_EQ(inputs[0].make_code, 0x1Eu);
	EXPECT_EQ(inputs[0].virtual_key, 0x41u);
	EXPECT_FALSE(inputs[0].is_extended);
	EXPECT_FALSE(inputs[0].is_extended1);

	EXPECT_FALSE(inputs[1].is_down);
	EXPECT_EQ(inputs[1].make_code, 0x1Eu);
}

TEST(RawKeyboardTranslateTest, ExtendedFlag)
{
	constexpr RawKeyboardRecord kRecords[] =
	{
		{ 0x001D, kE0, 0x0011 },	//	右 Ctrl
	};
	const auto inputs = TranslateSequence(kRecords);
	ASSERT_EQ(inputs.size(), 1u);
	EXPECT_EQ(inputs[0].make_code, 0x1Du);
	EXPECT_TRUE(inputs[0].is_extended);
	EXPECT_FALSE(inputs[0].is_extended1);
}

TEST(RawKeyboardTranslateTest, PauseTailIsDropped)
{
	//	Pause は E1 1D 45 E1 9D C5 を押した瞬間にまとめて送る
	constexpr RawKeyboardRecord kRecords[] =
	{
		{ 0x001D, kE1, 0x0013 },
		{ 0x0045, 0x0000, 0x00FF },
		{ 0x001D, kE1 | kBreak, 0x0013 },
		{ 0x0045, kBreak, 0x00FF },
	};
	const auto inputs = TranslateSequence(kRecords);
	ASSERT_EQ(inputs.size(), 2u);

	EXPECT_TRUE(inputs[0].is_down);
	EXPECT_EQ(inputs[0].make_code, 0x1Du);
	EXPECT_TRUE(inputs[0].is_extended1);
	EXPECT_EQ(nox::hid::MapKeyCode(inputs[0]), nox::hid::KeyCode::Pause);

	EXPECT_FALSE(inputs[1].is_down);
	EXPECT_EQ(nox::hid::MapKeyCode(inputs[1]), nox::hid::KeyCode::Pause);
}

TEST(RawKeyboardTranslateTest, NumLockIsKept)
{
	constexpr RawKeyboardRecord kRecords[] =
	{
		{ 0x0045, 0x0000, 0x0090 },
		{ 0x0045, kBreak, 0x0090 },
	};
	const auto inputs = TranslateSequence(kRecords);
	ASSERT_EQ(inputs.size(), 2u);
	EXPECT_EQ(nox::hid::MapKeyCode(inputs[0]), nox::hid::KeyCode::NumLock);
	EXPECT_EQ(nox::hid::MapKeyCode(inputs[1]), nox::hid::KeyCode::NumLock);
}

TEST(RawKeyboardTranslateTest, E1PendingIsClearedByNextInput)
{
	//	E1 の直後でなければ 45 は NumLock として送る
	constexpr RawKeyboardRecord kRecords[] =
	{
		{ 0x001D, kE1, 0x0013 },
		{ 0x001E, 0x0000, 0x0041 },
		{ 0x0045, 0x0000, 0x0090 },
	};
	const auto inputs = TranslateSequence(kRecords);
	ASSERT_EQ(inputs.size(), 3u);
	EXPECT_EQ(nox::hid::MapKeyCode(inputs[2]), nox::hid::KeyCode::NumLock);
}

//	---------------------------------------------------------------------------
//	従来のキーメッセージからの変換 (Raw Input を登録できなかったときの代替)
//	---------------------------------------------------------------------------

TEST(LegacyKeyMessageTest, PauseIsNormalizedToE1)
{
	//	従来のキーメッセージの Pause は 0x45 (拡張ビットなし)
	const nox::hid::RawKeyboardInput input = nox::hid::TranslateLegacyKeyMessage(true, 0x13u, 0x00450001u);
	EXPECT_TRUE(input.is_down);
	EXPECT_EQ(input.make_code, 0x1Du);
	EXPECT_FALSE(input.is_extended);
	EXPECT_TRUE(input.is_extended1);
	EXPECT_EQ(nox::hid::MapKeyCode(input), nox::hid::KeyCode::Pause);
}

TEST(LegacyKeyMessageTest, NumLockDropsExtendedBit)
{
	//	従来のキーメッセージの NumLock は 0x45 + 拡張ビット
	const nox::hid::RawKeyboardInput input = nox::hid::TranslateLegacyKeyMessage(true, 0x90u, 0x01450001u);
	EXPECT_EQ(input.make_code, 0x45u);
	EXPECT_FALSE(input.is_extended);
	EXPECT_FALSE(input.is_extended1);
	EXPECT_EQ(nox::hid::MapKeyCode(input), nox::hid::KeyCode::NumLock);
}

TEST(LegacyKeyMessageTest, ExtendedKeyUp)
{
	//	上矢印の解放 (lParam の bit30 / bit31 も立つ)
	const nox::hid::RawKeyboardInput input = nox::hid::TranslateLegacyKeyMessage(false, 0x26u, 0xC1480001u);
	EXPECT_FALSE(input.is_down);
	EXPECT_EQ(input.make_code, 0x48u);
	EXPECT_EQ(input.virtual_key, 0x26u);
	EXPECT_TRUE(input.is_extended);
	EXPECT_FALSE(input.is_extended1);
	EXPECT_EQ(nox::hid::MapKeyCode(input), nox::hid::KeyCode::UpArrow);
}

//	---------------------------------------------------------------------------
//	キーコードの対応表
//	---------------------------------------------------------------------------

TEST(KeyCodeMapTest, BasicKeys)
{
	EXPECT_EQ(MapRaw(0x001E, 0x0000, 0x0041), nox::hid::KeyCode::A);
	EXPECT_EQ(MapRaw(0x0039, 0x0000, 0x0020), nox::hid::KeyCode::Space);
	EXPECT_EQ(MapRaw(0x0001, 0x0000, 0x001B), nox::hid::KeyCode::Escape);
	EXPECT_EQ(MapRaw(0x001C, 0x0000, 0x000D), nox::hid::KeyCode::Enter);
	EXPECT_EQ(MapRaw(0x003B, 0x0000, 0x0070), nox::hid::KeyCode::F1);
	EXPECT_EQ(MapRaw(0x0058, 0x0000, 0x007B), nox::hid::KeyCode::F12);
}

TEST(KeyCodeMapTest, ModifiersAreDistinguishedBySide)
{
	EXPECT_EQ(MapRaw(0x001D, 0x0000, 0x0011), nox::hid::KeyCode::LeftControl);
	EXPECT_EQ(MapRaw(0x001D, kE0, 0x0011), nox::hid::KeyCode::RightControl);
	EXPECT_EQ(MapRaw(0x002A, 0x0000, 0x0010), nox::hid::KeyCode::LeftShift);
	EXPECT_EQ(MapRaw(0x0036, 0x0000, 0x0010), nox::hid::KeyCode::RightShift);
	EXPECT_EQ(MapRaw(0x0038, 0x0000, 0x0012), nox::hid::KeyCode::LeftAlt);
	EXPECT_EQ(MapRaw(0x0038, kE0, 0x0012), nox::hid::KeyCode::RightAlt);
	EXPECT_EQ(MapRaw(0x005B, kE0, 0x005B), nox::hid::KeyCode::LeftGui);
	EXPECT_EQ(MapRaw(0x005C, kE0, 0x005C), nox::hid::KeyCode::RightGui);
}

TEST(KeyCodeMapTest, KeypadAndNavigationAreSeparate)
{
	//	NumLock オフのテンキー 7 は VK_HOME で届くが、物理キーはテンキー 7
	EXPECT_EQ(MapRaw(0x0047, 0x0000, 0x0024), nox::hid::KeyCode::Keypad7);
	EXPECT_EQ(MapRaw(0x0047, kE0, 0x0024), nox::hid::KeyCode::Home);
	EXPECT_EQ(MapRaw(0x001C, kE0, 0x000D), nox::hid::KeyCode::KeypadEnter);
	EXPECT_EQ(MapRaw(0x0035, kE0, 0x006F), nox::hid::KeyCode::KeypadDivide);
	EXPECT_EQ(MapRaw(0x0048, kE0, 0x0026), nox::hid::KeyCode::UpArrow);
}

TEST(KeyCodeMapTest, SpecialSequences)
{
	EXPECT_EQ(MapRaw(0x0046, kE0, 0x0003), nox::hid::KeyCode::Pause);			//	Ctrl+Pause (Break)
	EXPECT_EQ(MapRaw(0x0037, kE0, 0x002C), nox::hid::KeyCode::PrintScreen);
	EXPECT_EQ(MapRaw(0x0054, 0x0000, 0x002C), nox::hid::KeyCode::PrintScreen);	//	Alt+PrintScreen (SysRq)
	EXPECT_EQ(MapRaw(0x002A, kE0, 0x00FF), nox::hid::KeyCode::Unknown);			//	キーボードが挟む偽の Shift
}

TEST(KeyCodeMapTest, IsoAndJisKeys)
{
	EXPECT_EQ(MapRaw(0x0056, 0x0000, 0x00E2), nox::hid::KeyCode::NonUsBackslash);	//	ISO の Z の左
	EXPECT_EQ(MapRaw(0x0073, 0x0000, 0x00E2), nox::hid::KeyCode::International1);	//	ろ (VK は VK_OEM_102)
	EXPECT_EQ(MapRaw(0x0070, 0x0000, 0x00F2), nox::hid::KeyCode::International2);	//	カタカナ/ひらがな
	EXPECT_EQ(MapRaw(0x007D, 0x0000, 0x00DC), nox::hid::KeyCode::International3);	//	¥
	EXPECT_EQ(MapRaw(0x0079, 0x0000, 0x001C), nox::hid::KeyCode::International4);	//	変換
	EXPECT_EQ(MapRaw(0x007B, 0x0000, 0x001D), nox::hid::KeyCode::International5);	//	無変換
	EXPECT_EQ(MapRaw(0x0029, 0x0000, 0x00F3), nox::hid::KeyCode::Grave);				//	半角/全角
}

TEST(KeyCodeMapTest, ExtendedFunctionAndMediaKeys)
{
	EXPECT_EQ(MapRaw(0x0064, 0x0000, 0x007C), nox::hid::KeyCode::F13);
	EXPECT_EQ(MapRaw(0x006E, 0x0000, 0x0086), nox::hid::KeyCode::F23);
	EXPECT_EQ(MapRaw(0x0076, 0x0000, 0x0087), nox::hid::KeyCode::F24);
	EXPECT_EQ(MapRaw(0x0020, kE0, 0x00AD), nox::hid::KeyCode::Mute);
	EXPECT_EQ(MapRaw(0x002E, kE0, 0x00AE), nox::hid::KeyCode::VolumeDown);
	EXPECT_EQ(MapRaw(0x0030, kE0, 0x00AF), nox::hid::KeyCode::VolumeUp);
}

TEST(KeyCodeMapTest, VirtualKeyFallbackWithoutScanCode)
{
	//	スキャンコードなしで注入された入力は仮想キーコードで補う
	EXPECT_EQ(MapRaw(0x0000, 0x0000, 0x0070), nox::hid::KeyCode::F1);
	EXPECT_EQ(MapRaw(0x0000, 0x0000, 0x007C), nox::hid::KeyCode::F13);
	EXPECT_EQ(MapRaw(0x0000, 0x0000, 0x0087), nox::hid::KeyCode::F24);
	EXPECT_EQ(MapRaw(0x0000, 0x0000, 0x00AD), nox::hid::KeyCode::Mute);
	//	修飾キーは左右が分からないので補わない
	EXPECT_EQ(MapRaw(0x0000, 0x0000, 0x0010), nox::hid::KeyCode::Unknown);
}
