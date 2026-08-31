// Copyright (C) 2026 NOX ENGINE All rights reserved.

/// @file	entity_logic.h
/// @brief	entity_logic
#pragma once
#include	"object.h"
#include	"entity.h"

namespace nox
{
	/// @brief oop的なEntityの振る舞いを定義するための基底クラス
	class EntityLogic : ::nox::Object
	{
		NOX_DECLARE_OBJECT(EntityLogic, nox::Object);
	public:
		inline constexpr explicit EntityLogic(const nox::EntityId& entity)noexcept:
			entity_(entity)
		{
		}

	protected:
		const nox::EntityId entity_;
	};
}