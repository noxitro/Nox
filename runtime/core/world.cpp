// Copyright (C) 2026 NOX ENGINE All rights reserved.

/// @file	world.cpp
/// @brief	world
#include "pch.h"
#include "world.h"
#include "system.h"

namespace nox
{
	namespace
	{
		[[nodiscard]]
		constexpr bool is_live_generation(nox::uint32 generation) noexcept
		{
			return (generation & 1u) != 0u;
		}

		[[nodiscard]]
		constexpr nox::uint32 get_entity_record_page_index(nox::uint32 index, nox::uint32 entity_record_page_shift) noexcept
		{
			return index >> entity_record_page_shift;
		}

		[[nodiscard]]
		constexpr nox::uint32 get_entity_record_offset(nox::uint32 index, nox::uint32 entity_record_page_mask) noexcept
		{
			return index & entity_record_page_mask;
		}

		[[nodiscard]]
		constexpr nox::uint64 make_free_entity_head(nox::uint32 index, nox::uint32 version) noexcept
		{
			return (static_cast<nox::uint64>(version) << 32u) | static_cast<nox::uint64>(index);
		}

		[[nodiscard]]
		constexpr nox::uint32 get_free_entity_head_index(nox::uint64 head) noexcept
		{
			return static_cast<nox::uint32>(head & 0xffffffffull);
		}

		[[nodiscard]]
		constexpr nox::uint32 get_free_entity_head_version(nox::uint64 head) noexcept
		{
			return static_cast<nox::uint32>(head >> 32u);
		}
	}

	struct World::Archtype
	{

	};
}

nox::World::World():
	free_entity_head_(make_free_entity_head(k_invalid_entity_index, 0u)),
	next_entity_index_(0u),
	first_entity_record_page_(),
	entity_record_pages_{},
	entity_record_page_mutex_(),
	studio_mode_(false),
	kill_(false),
	enabled_vsync_(true)
{
	for (auto&& entity_record_page : entity_record_pages_)
	{
		entity_record_page.store(nullptr, std::memory_order_relaxed);
	}

	entity_record_pages_[0].store(&first_entity_record_page_, std::memory_order_relaxed);
}

nox::World::~World()
{
	for (nox::uint32 page_index = 1u; page_index < k_max_entity_page_count; ++page_index)
	{
		delete entity_record_pages_[page_index].load(std::memory_order_relaxed);
	}
}

nox::World::EntityRecord* nox::World::TryGetEntityRecord(nox::uint32 index) noexcept
{
	const nox::uint32 page_index = get_entity_record_page_index(index, k_entity_record_page_shift);
	if (page_index >= k_max_entity_page_count)
	{
		return nullptr;
	}

	auto* entity_record_page = entity_record_pages_[page_index].load(std::memory_order_acquire);
	if (entity_record_page == nullptr)
	{
		return nullptr;
	}

	return &entity_record_page->records[get_entity_record_offset(index, k_entity_record_page_mask)];
}

const nox::World::EntityRecord* nox::World::TryGetEntityRecord(nox::uint32 index) const noexcept
{
	return const_cast<nox::World*>(this)->TryGetEntityRecord(index);
}

nox::World::EntityRecord* nox::World::EnsureEntityRecord(nox::uint32 index)
{
	const nox::uint32 page_index = get_entity_record_page_index(index, k_entity_record_page_shift);
	if (page_index >= k_max_entity_page_count)
	{
		NOX_ASSERT(false, u8"World entity capacity exceeded: index={0}", index);
		std::abort();
	}

	auto* entity_record_page = entity_record_pages_[page_index].load(std::memory_order_acquire);
	if (entity_record_page == nullptr)
	{
		std::lock_guard<std::mutex> lock(entity_record_page_mutex_);
		entity_record_page = entity_record_pages_[page_index].load(std::memory_order_relaxed);
		if (entity_record_page == nullptr)
		{
			entity_record_page = new EntityRecordPage();
			entity_record_pages_[page_index].store(entity_record_page, std::memory_order_release);
		}
	}

	return &entity_record_page->records[get_entity_record_offset(index, k_entity_record_page_mask)];
}

nox::uint32 nox::World::TryPopFreeEntityIndex() noexcept
{
	nox::uint64 head = free_entity_head_.load(std::memory_order_acquire);
	while (true)
	{
		const nox::uint32 index = get_free_entity_head_index(head);
		if (index == k_invalid_entity_index)
		{
			return k_invalid_entity_index;
		}

		auto* entity_record = TryGetEntityRecord(index);
		NOX_ASSERT(entity_record != nullptr, u8"Invalid free entity slot: index={0}", index);
		if (entity_record == nullptr)
		{
			return k_invalid_entity_index;
		}

		const nox::uint32 next_index = entity_record->next_free_index.load(std::memory_order_relaxed);
		const nox::uint64 next_head = make_free_entity_head(next_index, get_free_entity_head_version(head) + 1u);
		if (free_entity_head_.compare_exchange_weak(head, next_head, std::memory_order_acq_rel, std::memory_order_acquire))
		{
			return index;
		}
	}
}

void nox::World::PushFreeEntityIndex(nox::uint32 index) noexcept
{
	auto* entity_record = TryGetEntityRecord(index);
	NOX_ASSERT(entity_record != nullptr, u8"Invalid free entity slot push: index={0}", index);
	if (entity_record == nullptr)
	{
		return;
	}

	nox::uint64 head = free_entity_head_.load(std::memory_order_acquire);
	while (true)
	{
		entity_record->next_free_index.store(get_free_entity_head_index(head), std::memory_order_relaxed);
		const nox::uint64 next_head = make_free_entity_head(index, get_free_entity_head_version(head) + 1u);
		if (free_entity_head_.compare_exchange_weak(head, next_head, std::memory_order_release, std::memory_order_acquire))
		{
			return;
		}
	}
}

nox::EntityId nox::World::CreateEntity()
{
	nox::uint32 index = TryPopFreeEntityIndex();
	if (index != k_invalid_entity_index)
	{
		auto* entity_record = TryGetEntityRecord(index);
		NOX_ASSERT(entity_record != nullptr, u8"Invalid recycled entity slot: index={0}", index);
		if (entity_record == nullptr)
		{
			std::abort();
		}

		const nox::uint32 current_generation = entity_record->generation.load(std::memory_order_relaxed);
		NOX_ASSERT(is_live_generation(current_generation) == false, u8"Recycled slot must be free: index={0}", index);
		const nox::uint32 next_generation = current_generation + 1u;
		NOX_ASSERT(next_generation != 0u, u8"Entity generation overflow: index={0}", index);

		entity_record->row = 0u;
		entity_record->archtype = nullptr;
		entity_record->next_free_index.store(k_invalid_entity_index, std::memory_order_relaxed);
		entity_record->generation.store(next_generation, std::memory_order_release);

		return nox::EntityId{
			.index = index,
			.generation = next_generation,
		};
	}

	index = next_entity_index_.fetch_add(1u, std::memory_order_relaxed);
	if (index >= k_max_entity_count)
	{
		NOX_ASSERT(false, u8"World entity capacity exceeded: index={0}", index);
		std::abort();
	}

	auto* entity_record = EnsureEntityRecord(index);
	entity_record->row = 0u;
	entity_record->archtype = nullptr;
	entity_record->next_free_index.store(k_invalid_entity_index, std::memory_order_relaxed);
	entity_record->generation.store(k_initial_live_generation, std::memory_order_release);

	return nox::EntityId{
		.index = index,
		.generation = k_initial_live_generation,
	};
}

void nox::World::DestroyEntity(nox::EntityId entity)
{
	auto* entity_record = TryGetEntityRecord(entity.index);
	if (entity_record == nullptr)
	{
		return;
	}

	nox::uint32 expected_generation = entity.generation;
	if (is_live_generation(expected_generation) == false)
	{
		return;
	}

	const nox::uint32 next_generation = expected_generation + 1u;
	NOX_ASSERT(next_generation != 0u, u8"Entity generation overflow: index={0}", entity.index);

	if (entity_record->generation.compare_exchange_strong(expected_generation, next_generation, std::memory_order_acq_rel, std::memory_order_acquire) == false)
	{
		return;
	}

	entity_record->row = 0u;
	entity_record->archtype = nullptr;
	PushFreeEntityIndex(entity.index);
}

bool nox::World::IsAlive(nox::EntityId entity)const noexcept
{
	const auto* entity_record = TryGetEntityRecord(entity.index);
	if (entity_record == nullptr)
	{
		return false;
	}

	const nox::uint32 current_generation = entity_record->generation.load(std::memory_order_acquire);
	return is_live_generation(current_generation) && (current_generation == entity.generation);
}

void nox::World::Run()
{

}

void nox::World::Init()
{

}

void nox::World::Update()
{

}

void nox::World::Terminate()
{
	kill_ = true;

	for (auto&& system : systems_)
	{
		delete system;
	}

	systems_.clear();
	systems_.shrink_to_fit();
}