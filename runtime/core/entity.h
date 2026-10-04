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

		inline constexpr auto operator<=>(const Entity&)const noexcept = default;

		inline constexpr nox::uint64 operator()()const noexcept
		{
			return std::bit_cast<nox::uint64>(*this);
		}
	};

	static_assert(sizeof(nox::Entity) == sizeof(nox::uint64));
	static_assert(std::is_trivially_copyable_v<nox::Entity>);
}

template<>
struct std::hash<nox::Entity>
{
	[[nodiscard]] constexpr std::size_t operator()(const nox::Entity entity)const noexcept
	{
		//	下位ビットに偏る世代番号を全ビットへ散らしてから返す。
		//	バケット数が2のべき乗でマスクするテーブルでも偏らないようにするため
		nox::uint64 x = entity();
		x ^= x >> 30u;
		x *= 0xbf58476d1ce4e5b9ull;
		x ^= x >> 27u;
		x *= 0x94d049bb133111ebull;
		x ^= x >> 31u;
		return static_cast<std::size_t>(x);
	}
};