//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	module_entry_category.h
///	@brief	module_entry_category
#pragma once

namespace nox
{
	enum class SystemPhaseType : nox::uint8
	{
		/// @brief 他のシステムに依存しない初期化処理
		Init = 0,
		/// @brief 更新の前に一度だけ呼び出される処理
		Start,
		/// @brief 毎フレーム呼び出される処理
		Update,
		/// @brief 終了処理
		Terminate,
		_Max
	};
}