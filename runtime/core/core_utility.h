// Copyright (C) 2026 NOX ENGINE All rights reserved.

/// @file	core_utility.h
/// @brief	core_utility
#pragma once

namespace nox::util
{
	/// @brief プロジェクトのルートディレクトリを取得する
	/// @return 
	std::u16string_view GetProjectDir()noexcept;
	std::u8string_view GetProjectDir(std::array<nox::char8, nox::os::k_max_path_length>& buffer)noexcept;
}