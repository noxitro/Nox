// Copyright (C) 2026 NOX ENGINE All rights reserved.

/// @file	entity_commands.cpp
/// @brief	entity_commands
#include "pch.h"
#include "entity_commands.h"
#include "world.h"

void nox::EntityCommands::Destroy(const nox::EntityId entity)noexcept
{
	//	フェーズ実行中の即時破棄はできないため、必ずコマンドバッファへ積む。
	(void)world_->QueueDestroyEntity(entity);
}

bool nox::EntityCommands::IsAlive(const nox::EntityId entity)const noexcept
{
	return world_->IsAlive(entity);
}
