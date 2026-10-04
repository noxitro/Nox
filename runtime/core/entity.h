// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

/// @file	entity.h
/// @brief	entity
#pragma once

namespace nox
{
	/// @brief Entityハンドル(64bit)。生成/破棄で世代番号が進み、stale handleを検出できる。
	/// @details 値型として配列にそのまま並べるため、代入可能かつtrivially copyableであること。
	struct Entity final
	{
		/// @brief 世代番号(破棄/再利用で更新され、stale handleの検出に使う)
		nox::uint32 generation;
		/// @brief スロット番号
		nox::uint32 index;

		inline constexpr bool operator<=>(const Entity&)const noexcept = default;

		inline constexpr nox::uint64 operator()()const noexcept
		{
			return std::bit_cast<nox::uint64>(*this);
		}
	};

	static_assert(sizeof(nox::Entity) == sizeof(nox::uint64));
	static_assert(std::is_trivially_copyable_v<nox::Entity>);
}
