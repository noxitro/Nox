// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

/// @file	core_utility.h
/// @brief	core_utility
#pragma once
#include	<array>
#include	<string_view>

namespace nox::util
{
	/// @brief プロジェクトのルートディレクトリを取得する
	/// @return 
	std::u16string_view GetProjectDir()noexcept;
	std::u8string_view GetProjectDir(std::array<nox::char8, nox::k_max_path_length>& buffer)noexcept;
}