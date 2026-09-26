//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	raw_keyboard.h
///	@brief	Raw Input / 従来のキーメッセージから HID のキーコードへの変換
///	@details	OS の API にも core にも依存しない。hid_test から直接試せるよう、
///				raw_keyboard.cpp はプリコンパイル済みヘッダーを使わない。
#pragma once
#include	"definitions.h"

namespace nox::hid
{
	/// @brief キーボード入力 1 件
	/// @details make_code は Raw Input と同じ表現 (Set 1 の make code。E0 / E1 接頭辞はフラグで持つ)。
	///			従来のキーメッセージから作る場合もこの表現へ揃える。
	struct RawKeyboardInput
	{
		nox::uint16 make_code;
		nox::uint16 virtual_key;
		bool is_down;
		bool is_extended;
		bool is_extended1;
	};

	/// @brief RAWKEYBOARD::Flags のビット。RI_KEY_BREAK / RI_KEY_E0 / RI_KEY_E1 と同じ値
	inline constexpr nox::uint16 kRawKeyBreak = 0x01u;
	inline constexpr nox::uint16 kRawKeyE0 = 0x02u;
	inline constexpr nox::uint16 kRawKeyE1 = 0x04u;

	/// @brief Raw Input の入力を変換するときに、次の入力へ持ち越す状態
	struct RawKeyboardTranslateState
	{
		/// @brief 直前の入力に E1 が付いていたか (Pause の後半を見分ける)
		bool is_e1_pending = false;
	};

	/// @brief RAWKEYBOARD の値から入力を作る
	/// @details Pause は E1 1D と 45 の 2 件に分かれて届く。後半の 45 は NumLock と同じ値なので送らない
	/// @return 送る入力があれば true
	[[nodiscard]] bool TranslateRawKeyboardInput(
		nox::uint16 make_code,
		nox::uint16 flags,
		nox::uint16 virtual_key,
		RawKeyboardTranslateState& state,
		RawKeyboardInput& out)noexcept;

	/// @brief 従来のキーメッセージ (WM_KEYDOWN など) から入力を作る
	/// @details Pause と NumLock はどちらも 0x45 で届くので、Raw Input の表現へ揃える
	/// @param key_data lParam の下位 32 ビット
	[[nodiscard]] RawKeyboardInput TranslateLegacyKeyMessage(
		bool is_down,
		nox::uint16 virtual_key,
		nox::uint32 key_data)noexcept;

	/// @brief 入力から HID のキーコードを引く
	/// @details 物理キーを表したいので、レイアウトに依存しないスキャンコードを主に使い、
	///			引けないときだけ仮想キーコードで補う
	[[nodiscard]] nox::hid::KeyCode MapKeyCode(const RawKeyboardInput& input)noexcept;
}
