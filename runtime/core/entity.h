// Copyright (C) 2026 NOX ENGINE All rights reserved.

/// @file	entity.h
/// @brief	entity
#pragma once

namespace nox
{
	/// @brief Entityハンドル(64bit)。生成/破棄で世代番号が進み、stale handleを検出できる。
	/// @details 値型として配列にそのまま並べるため、代入可能かつtrivially copyableであること。
	union EntityId
	{
		nox::uint64 raw;

		struct
		{
			/// @brief 世代番号(破棄/再利用で更新され、stale handleの検出に使う)
			nox::uint32 generation;

			/// @brief スロット番号
			nox::uint32 index;
		};

		[[nodiscard]] inline constexpr bool operator==(const nox::EntityId& other)const noexcept
		{
			return raw == other.raw;
		}
	};

	static_assert(sizeof(nox::EntityId) == sizeof(nox::uint64));
	static_assert(std::is_trivially_copyable_v<nox::EntityId>);
}
