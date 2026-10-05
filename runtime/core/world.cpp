//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	world.cpp
///	@brief	world
#include	"pch.h"
#include	"world.h"
#include	<bit>
#include	<bitset>
namespace nox
{
	namespace
	{
		/// @brief 空のArchetypeのID。Componentが1つもないEntityが所属する。
		inline constexpr nox::uint32 kEmptyArchetypeId = 0;
		inline constexpr nox::uint16 kArchetypeComponentMaxCount = 255;

		class ComponentTypeSet final
		{
			static constexpr nox::uint64 kGoldenRatio64 = 0x9e3779b97f4a7c15ull;
		public:
			static constexpr nox::uint8 Capacity = 255;
		public:
			inline constexpr ComponentTypeSet(std::span<const nox::uint16> component_id_list)noexcept:
				component_indices_([](std::span<const nox::uint16> component_id_list)constexpr noexcept->auto {
				std::array<nox::uint16, Capacity> result{};
				for (std::size_t i = 0; i < component_id_list.size(); ++i)
				{
					result[i] = component_id_list[i];
				}
				return result;
					}(component_id_list)),
				summary_count_(static_cast<nox::uint8>(component_id_list.size()))
			{

			}

			bool Insert(const nox::uint16 component_id)noexcept
			{

			}

			bool Insert(const std::span<const nox::uint16> component_id_list)noexcept
			{

			}

			bool Contains(const nox::uint16 component_id)const noexcept
			{

			}

		private:
			std::array<nox::uint16, Capacity> component_indices_;
			/// @brief	集合に入っている型bit boolmfilter
			std::bitset<64> summary_bits_;
			nox::uint8 summary_count_;
		};
	}
}

class nox::World::Archetype final
{
	struct Chunk final
	{

	};

	struct Edge final
	{
		nox::uint32 archetype_id : kArchetypeIdBitWidth;
		bool is_add_edge : 1u;
	};

public:
	explicit Archetype(const std::span<const nox::uint16> component_id_list)noexcept:
		archetype_id_(kInvalidArchetypeId),
		component_id_list_(component_id_list.begin(), component_id_list.end())
	{

	}

	inline constexpr std::span<const nox::uint16> GetComponentIdList()const noexcept
	{
		return component_id_list_;
	}

private:
	nox::uint32 archetype_id_ : kArchetypeIdBitWidth;
	const nox::Vector<nox::uint16> component_id_list_;

	/// @brief このArchetypeに接続しているArchetypeのIDリスト
	nox::Vector<typename nox::World::Archetype::Edge> edges_;

	nox::Vector<nox::Entity> entities_;
};

struct nox::World::Detail
{
	Detail()noexcept = delete;

	static inline nox::World::Archetype* AllocArchetype(std::size_t size)
	{

	}
};

nox::World::World():
	empty_archetype_buffer_{ std::byte{} }
{
	static_assert(sizeof(nox::World::Archetype) <= sizeof(decltype(empty_archetype_buffer_)), "空のArchetypeのバッファが小さすぎます");

	nox::World::Archetype*const empty_archetype = std::construct_at(reinterpret_cast<nox::World::Archetype*>(empty_archetype_buffer_.data()), std::span<const nox::uint16>{});
	archetypes_.emplace_back(empty_archetype);
}

nox::World::~World()
{
	std::destroy_at(reinterpret_cast<nox::World::Archetype*>(empty_archetype_buffer_.data()));
}

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

void* nox::World::AddComponent(const nox::Entity entity, const nox::reflection::Type& type)
{
	if (IsAlive(entity) == false)
	{
		return nullptr;
	}

	nox::World::EntityRecord& record = entity_records_.at(entity.index);
	
	return nullptr;
}