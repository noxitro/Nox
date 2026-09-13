// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

/// @file	entity_commands.cpp
/// @brief	entity_commands
#include "pch.h"
#include "entity_commands.h"
#include "world.h"

nox::EntityId nox::detail::CreateEntityDeferredOfWorld(nox::World& world)noexcept
{
	return world.CreateEntityDuringPhase();
}

void nox::detail::QueueDestroyEntityOfWorld(nox::World& world, const nox::EntityId entity)noexcept
{
	(void)world.QueueDestroyEntity(entity);
}

void nox::detail::QueueAddComponentOfWorld(
	nox::World& world,
	const nox::EntityId entity,
	const nox::ComponentTypeInfo& type_info,
	const void* const source)noexcept
{
	(void)world.QueueAddComponent(entity, type_info, source);
}

void nox::detail::QueueRemoveComponentOfWorld(
	nox::World& world,
	const nox::EntityId entity,
	const nox::ComponentTypeInfo& type_info)noexcept
{
	(void)world.QueueRemoveComponent(entity, type_info);
}

bool nox::detail::IsAliveOfWorld(const nox::World& world, const nox::EntityId entity)noexcept
{
	return world.IsAlive(entity);
}

void nox::EntityCommands::Destroy(const nox::EntityId entity)noexcept
{
	//	フェーズ実行中の即時破棄はできないため、必ずコマンドバッファへ積む。
	nox::detail::QueueDestroyEntityOfWorld(*world_, entity);
}

bool nox::EntityCommands::IsAlive(const nox::EntityId entity)const noexcept
{
	return nox::detail::IsAliveOfWorld(*world_, entity);
}
