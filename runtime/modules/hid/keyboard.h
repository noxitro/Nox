//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	keyboard.h
///	@brief	keyboard
#pragma once
#include	"definitions.h"

#include	<bitset>
#include	<cstddef>

namespace nox::hid
{
	class KeyboardManager;

	class Keyboard
	{
		friend class KeyboardManager;

	public:
		[[nodiscard]] bool IsDown(KeyCode key_code)const noexcept;
		[[nodiscard]] bool WasPressed(KeyCode key_code)const noexcept;
		[[nodiscard]] bool WasReleased(KeyCode key_code)const noexcept;

	private:
		static constexpr std::size_t kKeyCount = 256;

		void BeginFrame()noexcept;
		void ProcessKeyEvent(KeyCode key_code, bool is_down)noexcept;
		void ReleaseAllKeys()noexcept;

		std::bitset<kKeyCount> is_down_{};
		std::bitset<kKeyCount> was_pressed_{};
		std::bitset<kKeyCount> was_released_{};
	};
}
