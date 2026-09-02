// Copyright (C) 2026 NOX ENGINE All rights reserved.

/// @file	entity_logic.cpp
/// @brief	entity_logic
#include "pch.h"
#include "entity_logic.h"

//	型の購読はリフレクション生成コードが行うため、ここに実行時の登録処理は無い。

nox::EntityLogicStorage::EntityLogicStorage(const nox::EntityLogicTypeDescriptor& descriptor) :
	descriptor_(descriptor),
	instance_stride_(0u),
	blocks_(),
	free_instances_(),
	entries_()
{
	//	アラインメントに切り上げた間隔で並べる。ブロック先頭は最大アラインメントで確保する。
	const nox::uint32 alignment = std::max(descriptor.instance_alignment, 1u);
	instance_stride_ = (descriptor.instance_size + alignment - 1u) & ~(alignment - 1u);
}

nox::EntityLogicStorage::~EntityLogicStorage()
{
	for (const nox::EntityLogicStorage::Entry& entry : entries_)
	{
		descriptor_.destruct(entry.instance);
	}
	entries_.clear();

	for (nox::uint8* const block : blocks_)
	{
		nox::memory::Deallocate(block);
	}
	blocks_.clear();
}

void* nox::EntityLogicStorage::AcquireInstanceMemory()
{
	if (free_instances_.empty() == false)
	{
		void* const memory = free_instances_.back();
		free_instances_.pop_back();
		return memory;
	}

	//	ブロック単位でまとめて確保し、以降の生成はフリーリストから取る。
	auto* const block = static_cast<nox::uint8*>(nox::memory::Allocate(
		static_cast<size_t>(instance_stride_) * nox::EntityLogicStorage::k_instances_per_block,
		descriptor_.instance_alignment,
		nox::memory::InstanceType::Other));
	blocks_.push_back(block);

	free_instances_.reserve(free_instances_.size() + nox::EntityLogicStorage::k_instances_per_block);
	for (nox::uint32 instance_index = nox::EntityLogicStorage::k_instances_per_block; instance_index > 1u; --instance_index)
	{
		free_instances_.push_back(block + static_cast<size_t>(instance_index - 1u) * instance_stride_);
	}
	return block;
}

nox::int32 nox::EntityLogicStorage::FindEntrySlot(const nox::EntityId entity)const noexcept
{
	for (nox::uint32 entry_index = 0u; entry_index < entries_.size(); ++entry_index)
	{
		if (entries_[entry_index].entity.raw == entity.raw)
		{
			return static_cast<nox::int32>(entry_index);
		}
	}
	return -1;
}

bool nox::EntityLogicStorage::Contains(const nox::EntityId entity)const noexcept
{
	return FindEntrySlot(entity) >= 0;
}

void nox::EntityLogicStorage::CreateInstance(nox::World& world, const nox::EntityId entity)
{
	if (Contains(entity))
	{
		return;
	}

	void* const memory = AcquireInstanceMemory();
	void* const instance = descriptor_.construct(memory, world, entity);
	entries_.push_back(nox::EntityLogicStorage::Entry{ .entity = entity, .instance = instance });
}

void nox::EntityLogicStorage::DestroyInstance(const nox::EntityId entity)noexcept
{
	const nox::int32 entry_slot = FindEntrySlot(entity);
	if (entry_slot < 0)
	{
		return;
	}

	const nox::uint32 index = static_cast<nox::uint32>(entry_slot);
	descriptor_.destruct(entries_[index].instance);
	free_instances_.push_back(entries_[index].instance);

	entries_[index] = entries_.back();
	entries_.pop_back();
}
