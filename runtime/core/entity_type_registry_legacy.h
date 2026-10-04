// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

/// @file	entity_type_registry_legacy.h
/// @brief	購読済みEntitySystem / EntityLogic型の一覧。
/// @note	旧実装 (nox::legacy)。新しい ECS への書き直しが終わるまでの退避先で、移行後にこのファイルごと消す。
/// @details 実体はリフレクション生成コード(reflection_generated)が定義する。
///          ヘッダにクラスを定義するだけで表に載るため、登録用のマクロも静的初期化も要らない。
///          表は定数初期化された記述子ポインタの配列(.rdata)で、動的初期化は一切走らない。
#pragma once

namespace nox::legacy
{
	struct EntitySystemTypeDescriptor;
	struct EntityLogicTypeDescriptor;

	/// @brief 購読済みEntitySystem型の記述子一覧。
	[[nodiscard]] std::span<const nox::legacy::EntitySystemTypeDescriptor* const> GetEntitySystemTypes()noexcept;

	/// @brief 購読済みEntityLogic型の記述子一覧。
	[[nodiscard]] std::span<const nox::legacy::EntityLogicTypeDescriptor* const> GetEntityLogicTypes()noexcept;
}
