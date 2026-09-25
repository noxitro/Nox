//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	raw_keyboard_input_test.cpp
///	@brief	RAWKEYBOARD / 従来のキーメッセージからキーボード入力への変換テスト
///	@details	実機の入力列は、開発ビルドの runtime.exe を --log-raw-keyboard 付きで起動し、
///				デバッグ出力に出る「RawKeyboard { MakeCode, Flags, VKey }」を
///				RawKeyboardRecord の配列へそのまま貼って追加する。

#include "pch.h"
#include "../os/os.h"

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

	constexpr nox::uint16 kBreak = nox::os::detail::kRawKeyBreak;
	constexpr nox::uint16 kE0 = nox::os::detail::kRawKeyE0;
	constexpr nox::uint16 kE1 = nox::os::detail::kRawKeyE1;

	/// @brief 入力列を順に変換し、送られる入力だけを返す
	std::vector<nox::os::RawKeyboardInputEvent> TranslateSequence(const std::span<const RawKeyboardRecord> records)
	{
		std::vector<nox::os::RawKeyboardInputEvent> events;
		nox::os::detail::RawKeyboardTranslateState state{};
		for (const RawKeyboardRecord& record : records)
		{
			nox::os::RawKeyboardInputEvent event{};
			if (nox::os::detail::TranslateRawKeyboardInput(
				record.make_code,
				record.flags,
				record.virtual_key,
				state,
				event) == true)
			{
				events.push_back(event);
			}
		}
		return events;
	}
}

TEST(RawKeyboardInputTest, KeyDownAndUp)
{
	constexpr RawKeyboardRecord kRecords[] =
	{
		{ 0x001E, 0x0000, 0x0041 },	//	A 押下
		{ 0x001E, kBreak, 0x0041 },	//	A 解放
	};
	const auto events = TranslateSequence(kRecords);
	ASSERT_EQ(events.size(), 2u);

	EXPECT_EQ(events[0].type, nox::os::RawKeyboardInputType::KeyDown);
	EXPECT_EQ(events[0].make_code, 0x1Eu);
	EXPECT_EQ(events[0].virtual_key, 0x41u);
	EXPECT_FALSE(events[0].is_extended);
	EXPECT_FALSE(events[0].is_extended1);

	EXPECT_EQ(events[1].type, nox::os::RawKeyboardInputType::KeyUp);
	EXPECT_EQ(events[1].make_code, 0x1Eu);
}

TEST(RawKeyboardInputTest, ExtendedFlag)
{
	constexpr RawKeyboardRecord kRecords[] =
	{
		{ 0x001D, kE0, 0x0011 },	//	右 Ctrl
	};
	const auto events = TranslateSequence(kRecords);
	ASSERT_EQ(events.size(), 1u);
	EXPECT_EQ(events[0].make_code, 0x1Du);
	EXPECT_TRUE(events[0].is_extended);
	EXPECT_FALSE(events[0].is_extended1);
}

TEST(RawKeyboardInputTest, PauseTailIsDropped)
{
	//	Pause は E1 1D 45 E1 9D C5 を押した瞬間にまとめて送る
	constexpr RawKeyboardRecord kRecords[] =
	{
		{ 0x001D, kE1, 0x0013 },
		{ 0x0045, 0x0000, 0x00FF },
		{ 0x001D, kE1 | kBreak, 0x0013 },
		{ 0x0045, kBreak, 0x00FF },
	};
	const auto events = TranslateSequence(kRecords);
	ASSERT_EQ(events.size(), 2u);

	EXPECT_EQ(events[0].type, nox::os::RawKeyboardInputType::KeyDown);
	EXPECT_EQ(events[0].make_code, 0x1Du);
	EXPECT_TRUE(events[0].is_extended1);

	EXPECT_EQ(events[1].type, nox::os::RawKeyboardInputType::KeyUp);
	EXPECT_EQ(events[1].make_code, 0x1Du);
	EXPECT_TRUE(events[1].is_extended1);
}

TEST(RawKeyboardInputTest, NumLockIsKept)
{
	constexpr RawKeyboardRecord kRecords[] =
	{
		{ 0x0045, 0x0000, 0x0090 },
		{ 0x0045, kBreak, 0x0090 },
	};
	const auto events = TranslateSequence(kRecords);
	ASSERT_EQ(events.size(), 2u);
	EXPECT_EQ(events[0].make_code, 0x45u);
	EXPECT_FALSE(events[0].is_extended);
	EXPECT_FALSE(events[0].is_extended1);
}

TEST(RawKeyboardInputTest, E1PendingIsClearedByNextInput)
{
	//	E1 の直後でなければ 45 は NumLock として送る
	constexpr RawKeyboardRecord kRecords[] =
	{
		{ 0x001D, kE1, 0x0013 },
		{ 0x001E, 0x0000, 0x0041 },
		{ 0x0045, 0x0000, 0x0090 },
	};
	const auto events = TranslateSequence(kRecords);
	ASSERT_EQ(events.size(), 3u);
	EXPECT_EQ(events[2].make_code, 0x45u);
	EXPECT_FALSE(events[2].is_extended1);
}

TEST(RawKeyboardInputTest, LegacyPause)
{
	//	従来のキーメッセージの Pause は 0x45 (拡張ビットなし)
	const nox::os::RawKeyboardInputEvent event =
		nox::os::detail::TranslateLegacyKeyMessage(true, 0x13u, 0x00450001u);
	EXPECT_EQ(event.type, nox::os::RawKeyboardInputType::KeyDown);
	EXPECT_EQ(event.make_code, 0x1Du);
	EXPECT_FALSE(event.is_extended);
	EXPECT_TRUE(event.is_extended1);
}

TEST(RawKeyboardInputTest, LegacyNumLock)
{
	//	従来のキーメッセージの NumLock は 0x45 + 拡張ビット
	const nox::os::RawKeyboardInputEvent event =
		nox::os::detail::TranslateLegacyKeyMessage(true, 0x90u, 0x01450001u);
	EXPECT_EQ(event.make_code, 0x45u);
	EXPECT_FALSE(event.is_extended);
	EXPECT_FALSE(event.is_extended1);
}

TEST(RawKeyboardInputTest, LegacyExtendedKeyUp)
{
	//	上矢印の解放 (lParam の bit30 / bit31 も立つ)
	const nox::os::RawKeyboardInputEvent event =
		nox::os::detail::TranslateLegacyKeyMessage(false, 0x26u, 0xC1480001u);
	EXPECT_EQ(event.type, nox::os::RawKeyboardInputType::KeyUp);
	EXPECT_EQ(event.make_code, 0x48u);
	EXPECT_EQ(event.virtual_key, 0x26u);
	EXPECT_TRUE(event.is_extended);
	EXPECT_FALSE(event.is_extended1);
}
