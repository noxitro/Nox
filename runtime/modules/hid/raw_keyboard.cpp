//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	raw_keyboard.cpp
///	@brief	Raw Input / 従来のキーメッセージから HID のキーコードへの変換
///	@note	プリコンパイル済みヘッダーを使わない (hid.vcxproj で NotUsing)。core に依存させないため
#include	"raw_keyboard.h"

namespace
{
	///	@brief	スキャンコード (Set 1) から HID のキーコードを引く
	///	@details	物理キーを表したいので、レイアウトに依存しないスキャンコードを主に使う。
	[[nodiscard]] nox::hid::KeyCode KeyCodeFromScanCode(const nox::hid::RawKeyboardInput& input)noexcept
	{
		if (input.is_extended1 == true)
		{
			//	E1 が付くのは Pause だけ
			return nox::hid::KeyCode::Pause;
		}

		if (input.is_extended == true)
		{
			switch (input.make_code)
			{
			case 0x1Cu: return nox::hid::KeyCode::KeypadEnter;
			case 0x1Du: return nox::hid::KeyCode::RightControl;
			case 0x20u: return nox::hid::KeyCode::Mute;
			case 0x2Eu: return nox::hid::KeyCode::VolumeDown;
			case 0x30u: return nox::hid::KeyCode::VolumeUp;
			case 0x35u: return nox::hid::KeyCode::KeypadDivide;
			case 0x37u: return nox::hid::KeyCode::PrintScreen;
			case 0x38u: return nox::hid::KeyCode::RightAlt;
			case 0x46u: return nox::hid::KeyCode::Pause;		//	Ctrl+Pause (Break)
			case 0x47u: return nox::hid::KeyCode::Home;
			case 0x48u: return nox::hid::KeyCode::UpArrow;
			case 0x49u: return nox::hid::KeyCode::PageUp;
			case 0x4Bu: return nox::hid::KeyCode::LeftArrow;
			case 0x4Du: return nox::hid::KeyCode::RightArrow;
			case 0x4Fu: return nox::hid::KeyCode::End;
			case 0x50u: return nox::hid::KeyCode::DownArrow;
			case 0x51u: return nox::hid::KeyCode::PageDown;
			case 0x52u: return nox::hid::KeyCode::Insert;
			case 0x53u: return nox::hid::KeyCode::Delete;
			case 0x5Bu: return nox::hid::KeyCode::LeftGui;
			case 0x5Cu: return nox::hid::KeyCode::RightGui;
			case 0x5Du: return nox::hid::KeyCode::Application;
			case 0x5Eu: return nox::hid::KeyCode::Power;
			//	E0 2A / E0 AA などキーボードが挟む偽の Shift もここで捨てる
			default: return nox::hid::KeyCode::Unknown;
			}
		}

		switch (input.make_code)
		{
		case 0x01u: return nox::hid::KeyCode::Escape;
		case 0x02u: return nox::hid::KeyCode::Digit1;
		case 0x03u: return nox::hid::KeyCode::Digit2;
		case 0x04u: return nox::hid::KeyCode::Digit3;
		case 0x05u: return nox::hid::KeyCode::Digit4;
		case 0x06u: return nox::hid::KeyCode::Digit5;
		case 0x07u: return nox::hid::KeyCode::Digit6;
		case 0x08u: return nox::hid::KeyCode::Digit7;
		case 0x09u: return nox::hid::KeyCode::Digit8;
		case 0x0Au: return nox::hid::KeyCode::Digit9;
		case 0x0Bu: return nox::hid::KeyCode::Digit0;
		case 0x0Cu: return nox::hid::KeyCode::Minus;
		case 0x0Du: return nox::hid::KeyCode::Equal;
		case 0x0Eu: return nox::hid::KeyCode::Backspace;
		case 0x0Fu: return nox::hid::KeyCode::Tab;
		case 0x10u: return nox::hid::KeyCode::Q;
		case 0x11u: return nox::hid::KeyCode::W;
		case 0x12u: return nox::hid::KeyCode::E;
		case 0x13u: return nox::hid::KeyCode::R;
		case 0x14u: return nox::hid::KeyCode::T;
		case 0x15u: return nox::hid::KeyCode::Y;
		case 0x16u: return nox::hid::KeyCode::U;
		case 0x17u: return nox::hid::KeyCode::I;
		case 0x18u: return nox::hid::KeyCode::O;
		case 0x19u: return nox::hid::KeyCode::P;
		case 0x1Au: return nox::hid::KeyCode::LeftBracket;
		case 0x1Bu: return nox::hid::KeyCode::RightBracket;
		case 0x1Cu: return nox::hid::KeyCode::Enter;
		case 0x1Du: return nox::hid::KeyCode::LeftControl;
		case 0x1Eu: return nox::hid::KeyCode::A;
		case 0x1Fu: return nox::hid::KeyCode::S;
		case 0x20u: return nox::hid::KeyCode::D;
		case 0x21u: return nox::hid::KeyCode::F;
		case 0x22u: return nox::hid::KeyCode::G;
		case 0x23u: return nox::hid::KeyCode::H;
		case 0x24u: return nox::hid::KeyCode::J;
		case 0x25u: return nox::hid::KeyCode::K;
		case 0x26u: return nox::hid::KeyCode::L;
		case 0x27u: return nox::hid::KeyCode::Semicolon;
		case 0x28u: return nox::hid::KeyCode::Apostrophe;
		case 0x29u: return nox::hid::KeyCode::Grave;			//	JIS では 半角/全角
		case 0x2Au: return nox::hid::KeyCode::LeftShift;
		case 0x2Bu: return nox::hid::KeyCode::Backslash;		//	ISO の Enter 横 (NonUsHash) も同じ値で届く
		case 0x2Cu: return nox::hid::KeyCode::Z;
		case 0x2Du: return nox::hid::KeyCode::X;
		case 0x2Eu: return nox::hid::KeyCode::C;
		case 0x2Fu: return nox::hid::KeyCode::V;
		case 0x30u: return nox::hid::KeyCode::B;
		case 0x31u: return nox::hid::KeyCode::N;
		case 0x32u: return nox::hid::KeyCode::M;
		case 0x33u: return nox::hid::KeyCode::Comma;
		case 0x34u: return nox::hid::KeyCode::Period;
		case 0x35u: return nox::hid::KeyCode::Slash;
		case 0x36u: return nox::hid::KeyCode::RightShift;
		case 0x37u: return nox::hid::KeyCode::KeypadMultiply;
		case 0x38u: return nox::hid::KeyCode::LeftAlt;
		case 0x39u: return nox::hid::KeyCode::Space;
		case 0x3Au: return nox::hid::KeyCode::CapsLock;
		case 0x3Bu: return nox::hid::KeyCode::F1;
		case 0x3Cu: return nox::hid::KeyCode::F2;
		case 0x3Du: return nox::hid::KeyCode::F3;
		case 0x3Eu: return nox::hid::KeyCode::F4;
		case 0x3Fu: return nox::hid::KeyCode::F5;
		case 0x40u: return nox::hid::KeyCode::F6;
		case 0x41u: return nox::hid::KeyCode::F7;
		case 0x42u: return nox::hid::KeyCode::F8;
		case 0x43u: return nox::hid::KeyCode::F9;
		case 0x44u: return nox::hid::KeyCode::F10;
		case 0x45u: return nox::hid::KeyCode::NumLock;
		case 0x46u: return nox::hid::KeyCode::ScrollLock;
		case 0x47u: return nox::hid::KeyCode::Keypad7;
		case 0x48u: return nox::hid::KeyCode::Keypad8;
		case 0x49u: return nox::hid::KeyCode::Keypad9;
		case 0x4Au: return nox::hid::KeyCode::KeypadSubtract;
		case 0x4Bu: return nox::hid::KeyCode::Keypad4;
		case 0x4Cu: return nox::hid::KeyCode::Keypad5;
		case 0x4Du: return nox::hid::KeyCode::Keypad6;
		case 0x4Eu: return nox::hid::KeyCode::KeypadAdd;
		case 0x4Fu: return nox::hid::KeyCode::Keypad1;
		case 0x50u: return nox::hid::KeyCode::Keypad2;
		case 0x51u: return nox::hid::KeyCode::Keypad3;
		case 0x52u: return nox::hid::KeyCode::Keypad0;
		case 0x53u: return nox::hid::KeyCode::KeypadDecimal;
		case 0x54u: return nox::hid::KeyCode::PrintScreen;		//	Alt+PrintScreen (SysRq)
		case 0x56u: return nox::hid::KeyCode::NonUsBackslash;	//	ISO の Z の左
		case 0x57u: return nox::hid::KeyCode::F11;
		case 0x58u: return nox::hid::KeyCode::F12;
		case 0x59u: return nox::hid::KeyCode::KeypadEqual;
		case 0x64u: return nox::hid::KeyCode::F13;
		case 0x65u: return nox::hid::KeyCode::F14;
		case 0x66u: return nox::hid::KeyCode::F15;
		case 0x67u: return nox::hid::KeyCode::F16;
		case 0x68u: return nox::hid::KeyCode::F17;
		case 0x69u: return nox::hid::KeyCode::F18;
		case 0x6Au: return nox::hid::KeyCode::F19;
		case 0x6Bu: return nox::hid::KeyCode::F20;
		case 0x6Cu: return nox::hid::KeyCode::F21;
		case 0x6Du: return nox::hid::KeyCode::F22;
		case 0x6Eu: return nox::hid::KeyCode::F23;
		case 0x70u: return nox::hid::KeyCode::International2;	//	カタカナ/ひらがな
		case 0x73u: return nox::hid::KeyCode::International1;	//	ろ
		case 0x76u: return nox::hid::KeyCode::F24;
		case 0x79u: return nox::hid::KeyCode::International4;	//	変換
		case 0x7Bu: return nox::hid::KeyCode::International5;	//	無変換
		case 0x7Du: return nox::hid::KeyCode::International3;	//	¥
		case 0x7Eu: return nox::hid::KeyCode::KeypadComma;
		default: return nox::hid::KeyCode::Unknown;
		}
	}

	///	@brief	仮想キーコードから HID のキーコードを引く
	///	@details	スキャンコードで引けなかったとき (スキャンコードなしで注入された入力など) の補助。
	///				レイアウトに依存するので、主には使わない。
	[[nodiscard]] nox::hid::KeyCode KeyCodeFromVirtualKey(const nox::uint16 virtual_key)noexcept
	{
		switch (virtual_key)
		{
		case 0x15u: return nox::hid::KeyCode::International2;	//	VK_KANA
		case 0x19u: return nox::hid::KeyCode::Language2;		//	VK_KANJI
		case 0x1Cu: return nox::hid::KeyCode::International4;	//	VK_CONVERT
		case 0x1Du: return nox::hid::KeyCode::International5;	//	VK_NONCONVERT
		case 0xADu: return nox::hid::KeyCode::Mute;				//	VK_VOLUME_MUTE
		case 0xAEu: return nox::hid::KeyCode::VolumeDown;		//	VK_VOLUME_DOWN
		case 0xAFu: return nox::hid::KeyCode::VolumeUp;			//	VK_VOLUME_UP
		case 0xE2u: return nox::hid::KeyCode::NonUsBackslash;	//	VK_OEM_102
		default: break;
		}

		//	VK_F1 - VK_F12
		if (virtual_key >= 0x70u && virtual_key <= 0x7Bu)
		{
			return static_cast<nox::hid::KeyCode>(
				static_cast<nox::uint8>(nox::hid::KeyCode::F1) + (virtual_key - 0x70u));
		}
		//	VK_F13 - VK_F24
		if (virtual_key >= 0x7Cu && virtual_key <= 0x87u)
		{
			return static_cast<nox::hid::KeyCode>(
				static_cast<nox::uint8>(nox::hid::KeyCode::F13) + (virtual_key - 0x7Cu));
		}
		return nox::hid::KeyCode::Unknown;
	}
}

bool nox::hid::TranslateRawKeyboardInput(
	const nox::uint16 make_code,
	const nox::uint16 flags,
	const nox::uint16 virtual_key,
	nox::hid::RawKeyboardTranslateState& state,
	nox::hid::RawKeyboardInput& out)noexcept
{
	const bool is_extended1 = (flags & nox::hid::kRawKeyE1) != 0u;

	//	Pause としては前半の E1 1D で送っている
	const bool is_pause_tail = (state.is_e1_pending == true) && (is_extended1 == false) && (make_code == 0x45u);
	state.is_e1_pending = is_extended1;
	if (is_pause_tail == true)
	{
		return false;
	}

	out = nox::hid::RawKeyboardInput
	{
		.make_code = make_code,
		.virtual_key = virtual_key,
		.is_down = (flags & nox::hid::kRawKeyBreak) == 0u,
		.is_extended = (flags & nox::hid::kRawKeyE0) != 0u,
		.is_extended1 = is_extended1
	};
	return true;
}

nox::hid::RawKeyboardInput nox::hid::TranslateLegacyKeyMessage(
	const bool is_down,
	const nox::uint16 virtual_key,
	const nox::uint32 key_data)noexcept
{
	nox::uint16 make_code = static_cast<nox::uint16>((key_data >> 16u) & 0xFFu);
	bool is_extended = (key_data & (1u << 24u)) != 0u;
	bool is_extended1 = false;

	//	従来のキーメッセージでは Pause と NumLock がどちらも 0x45 で、拡張ビットの有無だけが違う。
	//	Raw Input の表現 (Pause は E1 1D、NumLock は拡張なしの 45) へ揃える。
	if (make_code == 0x45u)
	{
		if (is_extended == true)
		{
			is_extended = false;
		}
		else
		{
			make_code = 0x1Du;
			is_extended1 = true;
		}
	}

	return nox::hid::RawKeyboardInput
	{
		.make_code = make_code,
		.virtual_key = virtual_key,
		.is_down = is_down,
		.is_extended = is_extended,
		.is_extended1 = is_extended1
	};
}

nox::hid::KeyCode nox::hid::MapKeyCode(const nox::hid::RawKeyboardInput& input)noexcept
{
	const nox::hid::KeyCode key_code = KeyCodeFromScanCode(input);
	if (key_code != nox::hid::KeyCode::Unknown)
	{
		return key_code;
	}
	return KeyCodeFromVirtualKey(input.virtual_key);
}
