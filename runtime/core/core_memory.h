//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	core_memory.h
///	@brief	core_memory
#pragma once

namespace nox
{
	class Object;

	namespace memory
	{
		nox::Object* AsCastObject(nox::not_null<void*> addr);

		/// @brief		メモリリークチェック
		/// @details	kernel版と違って、nox::Object型の詳細を表示する
		void	CheckMemoryLeakCore();

		class MemoryProfilerCore
		{

		};
	}
}