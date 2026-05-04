//	Copyright (C) 2024 NOX ENGINE All Rights Rserved.

///	@file	entry_point.h
///	@brief	entry_point
#pragma once

namespace nox
{
	/// @brief エントリポイント
	/// @param args 引数
	nox::int32 EntryPoint(const std::span<const nox::char16* const> args);
}