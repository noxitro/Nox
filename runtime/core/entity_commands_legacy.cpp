// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

/// @file	entity_commands_legacy.cpp
/// @brief	entity_commands
#include "pch.h"
#include "entity_commands_legacy.h"
#include "world_legacy.h"

nox::Entity nox::legacy::detail::CreateEntityDeferredOfWorld(nox::legacy::World& world)noexcept
{
	return world.CreateEntityDuringPhase();
}

void nox::legacy::detail::QueueDestroyEntityOfWorld(nox::legacy::World& world, const nox::Entity entity)noexcept
{
	(void)world.QueueDestroyEntity(entity);
}

void nox::legacy::detail::QueueAddComponentOfWorld(
	nox::legacy::World& world,
	const nox::Entity entity,
	const nox::ComponentTypeInfo& type_info,
	const void* const source)noexcept
{
	(void)world.QueueAddComponent(entity, type_info, source);
}

void nox::legacy::detail::QueueRemoveComponentOfWorld(
	nox::legacy::World& world,
	const nox::Entity entity,
	const nox::ComponentTypeInfo& type_info)noexcept
{
	(void)world.QueueRemoveComponent(entity, type_info);
}

bool nox::legacy::detail::IsAliveOfWorld(const nox::legacy::World& world, const nox::Entity entity)noexcept
{
	return world.IsAlive(entity);
}

void nox::legacy::EntityCommands::Destroy(const nox::Entity entity)noexcept
{
	//	フェーズ実行中の即時破棄はできないため、必ずコマンドバッファへ積む。
	nox::legacy::detail::QueueDestroyEntityOfWorld(*world_, entity);
}

bool nox::legacy::EntityCommands::IsAlive(const nox::Entity entity)const noexcept
{
	return nox::legacy::detail::IsAliveOfWorld(*world_, entity);
}
