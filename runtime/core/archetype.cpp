// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

/// @file	archetype.cpp
/// @brief	archetype
#include "pch.h"
#include "archetype.h"

namespace
{
	[[nodiscard]]
	constexpr nox::uint32 align_up(const nox::uint32 value, const nox::uint32 alignment) noexcept
	{
		return (value + alignment - 1u) & ~(alignment - 1u);
	}
}

nox::Archetype::Archetype(const nox::ComponentMask& mask, std::span<const nox::ComponentTypeInfo* const> component_types) :
	mask_(mask),
	type_indices_{},
	column_offsets_{},
	component_sizes_{},
	chunks_(),
	type_count_(static_cast<nox::uint32>(component_types.size())),
	chunk_capacity_(0u),
	entity_count_(0u)
{
	NOX_ASSERT(type_count_ <= nox::k_max_component_type_per_archetype,
		u8"1Archetypeが持てるComponentData型数の上限({0})を超えました", nox::k_max_component_type_per_archetype);
	if (type_count_ > nox::k_max_component_type_per_archetype)
	{
		std::abort();
	}

	//	ComponentTypeIndexの昇順に並べる。Archetypeの同一性とマスクの走査順を一致させるため。
	std::array<const nox::ComponentTypeInfo*, nox::k_max_component_type_per_archetype> sorted_types{};
	for (nox::uint32 type_slot = 0u; type_slot < type_count_; ++type_slot)
	{
		sorted_types[type_slot] = component_types[type_slot];
	}
	std::sort(sorted_types.data(), sorted_types.data() + type_count_,
		[](const nox::ComponentTypeInfo* left, const nox::ComponentTypeInfo* right) noexcept
		{
			return left->index < right->index;
		});

	//	1行あたりのバイト数。EntityId列も同じ行数ぶん確保する。
	nox::uint32 row_stride = static_cast<nox::uint32>(sizeof(nox::EntityId));
	nox::uint32 max_alignment = static_cast<nox::uint32>(alignof(nox::EntityId));
	for (nox::uint32 type_slot = 0u; type_slot < type_count_; ++type_slot)
	{
		row_stride += sorted_types[type_slot]->size;
		max_alignment = std::max(max_alignment, sorted_types[type_slot]->alignment);
	}

	//	列ごとのアラインメント調整で最大 max_alignment * (列数+1) の余白が出る。
	const nox::uint32 alignment_budget = max_alignment * (type_count_ + 1u);
	NOX_ASSERT(nox::k_archetype_chunk_byte_size > alignment_budget + row_stride,
		u8"ComponentDataの合計サイズがChunkに収まりません");
	chunk_capacity_ = (nox::k_archetype_chunk_byte_size - alignment_budget) / row_stride;
	if (chunk_capacity_ == 0u)
	{
		std::abort();
	}

	nox::uint32 offset = align_up(chunk_capacity_ * static_cast<nox::uint32>(sizeof(nox::EntityId)), max_alignment);
	for (nox::uint32 type_slot = 0u; type_slot < type_count_; ++type_slot)
	{
		const nox::ComponentTypeInfo& type_info = *sorted_types[type_slot];
		offset = align_up(offset, type_info.alignment);
		type_indices_[type_slot] = type_info.index;
		column_offsets_[type_slot] = offset;
		component_sizes_[type_slot] = type_info.size;
		offset += chunk_capacity_ * type_info.size;
	}
	NOX_ASSERT(offset <= nox::k_archetype_chunk_byte_size, u8"Chunkのレイアウト計算が破綻しています");
}

nox::Archetype::~Archetype()
{
	for (nox::Archetype::Chunk& chunk : chunks_)
	{
		if (chunk.block != nullptr)
		{
			nox::memory::Deallocate(chunk.block);
			chunk.block = nullptr;
		}
	}
}

void nox::Archetype::PushChunk()
{
	auto* block = static_cast<nox::uint8*>(nox::memory::Allocate(
		nox::k_archetype_chunk_byte_size,
		alignof(std::max_align_t),
		nox::memory::InstanceType::Other));
	std::memset(block, 0, nox::k_archetype_chunk_byte_size);
	chunks_.push_back(nox::Archetype::Chunk{ .block = block, .count = 0u });
}

nox::ArchetypeLocation nox::Archetype::AddEntity(const nox::EntityId entity)
{
	//	末尾から空きのあるChunkを探す。削除はswap-removeなので末尾以外が空くことはない。
	if (chunks_.empty() || chunks_.back().count >= chunk_capacity_)
	{
		PushChunk();
	}

	const nox::uint32 chunk_index = static_cast<nox::uint32>(chunks_.size()) - 1u;
	nox::Archetype::Chunk& chunk = chunks_[chunk_index];
	const nox::uint32 row = chunk.count;

	reinterpret_cast<nox::EntityId*>(chunk.block)[row] = entity;
	for (nox::uint32 type_slot = 0u; type_slot < type_count_; ++type_slot)
	{
		std::memset(
			chunk.block + column_offsets_[type_slot] + static_cast<size_t>(row) * component_sizes_[type_slot],
			0,
			component_sizes_[type_slot]);
	}

	++chunk.count;
	++entity_count_;
	return nox::ArchetypeLocation{ .chunk_index = chunk_index, .row = row };
}

nox::EntityId nox::Archetype::RemoveEntity(const nox::ArchetypeLocation location)noexcept
{
	NOX_ASSERT(location.chunk_index < chunks_.size(), u8"Archetypeのchunk_indexが範囲外です");
	if (location.chunk_index >= chunks_.size())
	{
		return nox::EntityId{ 0u };
	}

	nox::Archetype::Chunk& target_chunk = chunks_[location.chunk_index];
	NOX_ASSERT(location.row < target_chunk.count, u8"Archetypeのrowが範囲外です");
	if (location.row >= target_chunk.count)
	{
		return nox::EntityId{ 0u };
	}

	//	常に「全体の末尾」を穴に移す。これで空きは末尾Chunkにしか生まれない。
	nox::Archetype::Chunk& last_chunk = chunks_.back();
	const nox::uint32 last_row = last_chunk.count - 1u;
	const bool is_self = (&target_chunk == &last_chunk) && (location.row == last_row);

	nox::EntityId moved_entity{ 0u };
	if (is_self == false)
	{
		moved_entity = reinterpret_cast<nox::EntityId*>(last_chunk.block)[last_row];
		reinterpret_cast<nox::EntityId*>(target_chunk.block)[location.row] = moved_entity;
		for (nox::uint32 type_slot = 0u; type_slot < type_count_; ++type_slot)
		{
			const nox::uint32 component_size = component_sizes_[type_slot];
			std::memcpy(
				target_chunk.block + column_offsets_[type_slot] + static_cast<size_t>(location.row) * component_size,
				last_chunk.block + column_offsets_[type_slot] + static_cast<size_t>(last_row) * component_size,
				component_size);
		}
	}

	--last_chunk.count;
	--entity_count_;
	if (last_chunk.count == 0u && chunks_.size() > 1u)
	{
		nox::memory::Deallocate(last_chunk.block);
		chunks_.pop_back();
	}
	return moved_entity;
}

void* nox::Archetype::TryGetComponentArray(const nox::uint32 chunk_index, const nox::ComponentTypeIndex type_index)noexcept
{
	const nox::int32 type_slot = FindTypeSlot(type_index);
	if (type_slot < 0 || chunk_index >= chunks_.size())
	{
		return nullptr;
	}
	return chunks_[chunk_index].block + column_offsets_[static_cast<nox::uint32>(type_slot)];
}

nox::EntityId* nox::Archetype::GetEntityArray(const nox::uint32 chunk_index)noexcept
{
	NOX_ASSERT(chunk_index < chunks_.size(), u8"Archetypeのchunk_indexが範囲外です");
	if (chunk_index >= chunks_.size())
	{
		return nullptr;
	}
	return reinterpret_cast<nox::EntityId*>(chunks_[chunk_index].block);
}

nox::uint32 nox::Archetype::GetChunkEntityCount(const nox::uint32 chunk_index)const noexcept
{
	if (chunk_index >= chunks_.size())
	{
		return 0u;
	}
	return chunks_[chunk_index].count;
}

void nox::Archetype::CopySharedComponents(
	const nox::ArchetypeLocation source_location,
	nox::Archetype& destination,
	const nox::ArchetypeLocation destination_location)noexcept
{
	//	双方ともComponentTypeIndexの昇順なので、マージ走査で共通の型だけを拾える。
	nox::uint32 source_slot = 0u;
	nox::uint32 destination_slot = 0u;
	while (source_slot < type_count_ && destination_slot < destination.type_count_)
	{
		const nox::ComponentTypeIndex source_type = type_indices_[source_slot];
		const nox::ComponentTypeIndex destination_type = destination.type_indices_[destination_slot];
		if (source_type < destination_type)
		{
			++source_slot;
			continue;
		}
		if (destination_type < source_type)
		{
			++destination_slot;
			continue;
		}

		const nox::uint32 component_size = component_sizes_[source_slot];
		std::memcpy(
			destination.chunks_[destination_location.chunk_index].block +
			destination.column_offsets_[destination_slot] +
			static_cast<size_t>(destination_location.row) * component_size,
			chunks_[source_location.chunk_index].block +
			column_offsets_[source_slot] +
			static_cast<size_t>(source_location.row) * component_size,
			component_size);
		++source_slot;
		++destination_slot;
	}
}

nox::int32 nox::Archetype::FindTypeSlot(const nox::ComponentTypeIndex type_index)const noexcept
{
	//	型数が少ないため線形走査で十分速い(分岐予測が効き、キャッシュライン1本に収まる)。
	for (nox::uint32 type_slot = 0u; type_slot < type_count_; ++type_slot)
	{
		if (type_indices_[type_slot] == type_index)
		{
			return static_cast<nox::int32>(type_slot);
		}
	}
	return -1;
}
