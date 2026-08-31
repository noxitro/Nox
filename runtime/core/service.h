// Copyright (C) 2026 NOX ENGINE All rights reserved.

/// @file	service.h
/// @brief	service
/// @details Worldが所有する共有機能。Worldの破棄がそのままServiceの破棄になるため、
///          シングルトンのような特別な終了処理・再初期化を持たない。
///          System / EntityLogic は引数に Service* を書くだけで受け取れる。
#pragma once
#include	"object.h"

namespace nox
{
	class World;

	/// @brief Worldに登録される共有機能の基底。
	class Service : public ::nox::Object
	{
		friend class World;
		NOX_DECLARE_OBJECT(Service, nox::Object);
	protected:
		inline constexpr Service()noexcept = default;
	};

	namespace detail
	{
		/// @brief Worldに登録済みのServiceを型で引く。
		/// @details entity_query.hがworld.hに依存しないための橋渡し。
		[[nodiscard]] nox::Service* TryGetServiceOfWorld(nox::World& world, const nox::reflection::Type& type)noexcept;
	}
}
