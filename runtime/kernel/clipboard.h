//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	clipboard.h
///	@brief	clipboard
#pragma once

#include	"basic_definition.h"
#include	"basic_type.h"
#include	"nox_string.h"
#include	"nox_string_view.h"

namespace nox::clipboard
{
	bool Clear();
	bool SetText(const std::u8string_view text);

	std::optional<nox::U16String> GetText();
	std::optional<nox::U16StringView> GetText(std::span<nox::char16> dest_buffer);
}