// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

/// @file	entity_system.h
/// @brief	System
#pragma once
#include	"entity_query.h"
#include	"system_phase_type.h"

namespace nox
{
	class World;
	class EntitySystemBase;

	/// @brief		ECSのSystem
	///				OnUpdate,OnAdd,OnRemoveを提供する
	/// @details	インスタンス化は不可。
	struct EntitySystemBase
	{

	};

	template<class T>
	struct EntitySystem
	{

	};
}