// Copyright (C) 2026 NOX ENGINE All rights reserved.

/// @file	entity_type_registry.h
/// @brief	購読済みEntitySystem / EntityLogic型の一覧。
/// @details 実体はリフレクション生成コード(reflection_generated)が定義する。
///          ヘッダにクラスを定義するだけで表に載るため、登録用のマクロも静的初期化も要らない。
///          表は定数初期化された記述子ポインタの配列(.rdata)で、動的初期化は一切走らない。
#pragma once

namespace nox
{
	struct EntitySystemTypeDescriptor;
	struct EntityLogicTypeDescriptor;

	/// @brief 購読済みEntitySystem型の記述子一覧。
	[[nodiscard]] std::span<const nox::EntitySystemTypeDescriptor* const> GetEntitySystemTypes()noexcept;

	/// @brief 購読済みEntityLogic型の記述子一覧。
	[[nodiscard]] std::span<const nox::EntityLogicTypeDescriptor* const> GetEntityLogicTypes()noexcept;
}
