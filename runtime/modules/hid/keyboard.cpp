//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	keyboard.cpp
///	@brief	keyboard
#include	"pch.h"
#include	"keyboard.h"

bool nox::hid::Keyboard::IsDown(const nox::hid::KeyCode key_code)const noexcept
{
	const std::size_t index = static_cast<nox::uint8>(key_code);
	return (index != 0u) && is_down_[index];
}

bool nox::hid::Keyboard::WasPressed(const nox::hid::KeyCode key_code)const noexcept
{
	const std::size_t index = static_cast<nox::uint8>(key_code);
	return (index != 0u) && was_pressed_[index];
}

bool nox::hid::Keyboard::WasReleased(const nox::hid::KeyCode key_code)const noexcept
{
	const std::size_t index = static_cast<nox::uint8>(key_code);
	return (index != 0u) && was_released_[index];
}

void nox::hid::Keyboard::BeginFrame()noexcept
{
	was_pressed_.reset();
	was_released_.reset();
}

void nox::hid::Keyboard::ProcessKeyEvent(const nox::hid::KeyCode key_code, const bool is_down)noexcept
{
	const std::size_t index = static_cast<nox::uint8>(key_code);
	if (index == 0u)
	{
		return;
	}

	if (is_down == true)
	{
		if (is_down_[index] == false)
		{
			was_pressed_[index] = true;
		}
		is_down_[index] = true;
	}
	else
	{
		if (is_down_[index] == true)
		{
			was_released_[index] = true;
		}
		is_down_[index] = false;
	}
}

void nox::hid::Keyboard::ReleaseAllKeys()noexcept
{
	was_released_ |= is_down_;
	is_down_.reset();
}
