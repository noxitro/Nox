//	Copyright (C) 2024 NOX ENGINE All Rights Rserved.

///	@file	module_entry_category.h
///	@brief	module_entry_category
#pragma once

namespace nox
{
	/// @brief モジュールエントリカテゴリ
	enum class ModuleEntryCategory : nox::uint8
	{
		//	init
		_Init = 0,

		CoreInit,

		//	setup
		_Setup,

		//	start
		_Start,

		//	update
		_Update,

		//	terminal
		_Terminal,

		//	finalize
		_Finalize,
	};
}