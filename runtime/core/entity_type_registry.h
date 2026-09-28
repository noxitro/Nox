// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

/// @file	entity_type_registry.h
/// @brief	購読済みEntitySystem / EntityLogic型、属性付きServiceメソッド、Taskの一覧。
/// @details 実体はリフレクション生成コード(reflection_generated)が定義する。
///          ヘッダにクラスを定義するだけで表に載るため、登録用のマクロも静的初期化も要らない。
///          表は定数初期化された記述子ポインタの配列(.rdata)で、動的初期化は一切走らない。
#pragma once

namespace nox
{
	struct EntitySystemTypeDescriptor;
	struct EntityLogicTypeDescriptor;
	struct ServiceMethodTypeDescriptor;
	struct UpdaterTaskDescriptor;

	/// @brief 購読済みEntitySystem型の記述子一覧。
	[[nodiscard]] std::span<const nox::EntitySystemTypeDescriptor* const> GetEntitySystemTypes()noexcept;

	/// @brief 購読済みEntityLogic型の記述子一覧。
	[[nodiscard]] std::span<const nox::EntityLogicTypeDescriptor* const> GetEntityLogicTypes()noexcept;

	/// @brief 属性付きメソッド(nox::attr::ServiceMethod)を持つService型の記述子一覧。
	/// @details Serviceのインスタンスは含まない。Worldは登録済みのServiceとだけ組にしてノードにする。
	[[nodiscard]] std::span<const nox::ServiceMethodTypeDescriptor* const> GetServiceMethodTypes()noexcept;

	/// @brief 属性付きのグローバル関数(nox::attr::UpdaterTask)の記述子一覧。
	[[nodiscard]] std::span<const nox::UpdaterTaskDescriptor* const> GetUpdaterTaskDescriptors()noexcept;
}
