//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	world.cpp
///	@brief	world
#include	"pch.h"
#include	"world.h"

nox::Entity nox::World::CreateEntity()
{
	if (free_entities_.empty())
	{
		const nox::uint32 entity_index = static_cast<nox::uint32>(entity_records_.size());
		entity_records_.emplace_back(nox::World::EntityRecord{});
		return nox::Entity{ .generation = 0u, .index = entity_index };
	}
	else
	{
		const nox::uint32 entity_index = free_entities_.back();
		free_entities_.pop_back();

		const nox::World::EntityRecord& record = entity_records_[entity_index];
		return nox::Entity{ .generation = record.generation, .index = entity_index };
	}
}

void nox::World::Delete(const nox::Entity entity)
{
	if (IsAlive(entity) == false)
	{
		return;
	}

	nox::World::EntityRecord& record = entity_records_.at(entity.index);
	++record.generation;
	free_entities_.emplace_back(entity.index);
}

bool nox::World::IsAlive(const nox::Entity entity)const noexcept
{
	NOX_ASSERT(entity.index < entity_records_.size(), u8"entityが範囲外です");
	return entity_records_.at(entity.index).generation == entity.generation;
}