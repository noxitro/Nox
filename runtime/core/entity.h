// Copyright (C) 2026 NOX ENGINE All rights reserved.

/// @file	entity.h
/// @brief	entity
#pragma once

namespace nox
{
	//	64bit
	union EntityId
	{
		const nox::uint64 raw;

		struct
		{
			/// @brief 世代番号(破棄/再利用で更新され、stale handleの検出に使う)
			nox::uint32 generation : 32;

			/// @brief スロット番号
			nox::uint32 index : 32;
		};
	};
}