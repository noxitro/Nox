//	Copyright (C) 2024 NOX ENGINE All Rights Rserved.

///	@file	module_entry_category.h
///	@brief	module_entry_category
#pragma once

namespace nox
{
	enum class ModulePhaseType : nox::uint8
	{
		Init = 0,
		Setup,
		Start,
		Update,
		Terminal,
		Finalize,
		 /// @brief 最大値
		_Max
	};

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

		SocketUpdate,
		GCUpdate,

		//	terminal
		_Terminal,

		//	finalize
		_Finalize,
		CoreFinalize,

		/// @brief 最大値
		_Max
	};
}