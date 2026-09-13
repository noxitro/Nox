//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	entry_point.h
///	@brief	entry_point
#pragma once

namespace nox
{
	/// @brief エントリポイント
	/// @param args 引数
	nox::int32 EntryPoint(const std::span<const nox::char16* const> args);
}