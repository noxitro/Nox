// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

/// @file	service_legacy.h
/// @brief	Worldが所有する共有機能 (旧実装)。
/// @note	 service.h の書き直しが終わるまでの退避先。World / EntitySystem / EntityLogic / テストはこちらを使う。
///			新しい Service へ移行したら、このファイルと nox::legacy::Service ごと消す。
/// @details Worldの破棄がそのままServiceの破棄になるため、
///          シングルトンのような特別な終了処理・再初期化を持たない。
///          System / EntityLogic は引数に Service* を書くだけで受け取れる。
#pragma once
#include	"object.h"

namespace nox
{
	class World;
}

namespace nox::legacy
{
	/// @brief Worldに登録される共有機能の基底。
	class Service : public ::nox::Object
	{
		friend class nox::World;
		NOX_DECLARE_OBJECT(Service, nox::Object);
	protected:
		inline constexpr Service()noexcept = default;
	};
}

namespace nox::detail
{
	/// @brief Worldに登録済みのServiceを型で引く。
	/// @details entity_query.hがworld.hに依存しないための橋渡し。
	[[nodiscard]] nox::legacy::Service* TryGetServiceOfWorld(nox::World& world, const nox::reflection::Type& type)noexcept;
}