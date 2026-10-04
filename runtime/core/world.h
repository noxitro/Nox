//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	world.h
///	@brief	world
#pragma once
#include	"entity.h"
#include	"component_id.h"

namespace nox
{
	class World final
	{
	private:
		struct Detail;
		/// @brief Componentの物理配置を表すArchetype
		class Archetype;

		struct EntityRecord final
		{
			constexpr EntityRecord()noexcept : 
				generation(0u), 
				archetype_id(std::numeric_limits<nox::uint32>::max()) 
			{}

			nox::uint32 generation;
			nox::uint32 archetype_id;
			
		};
	public:
		World();
		~World();

		nox::Entity CreateEntity();
		void Delete(nox::Entity entity);
		bool IsAlive(nox::Entity entity)const noexcept;

		void* AddComponent(const nox::Entity entity, const nox::reflection::Type& type);
		template<class T>
		inline T* AddComponent(const nox::Entity entity)
		{
			return static_cast<T*>(AddComponent(entity, nox::reflection::Typeof<T>()));
		}
	private:

	private:
		nox::Vector<EntityRecord> entity_records_;
		nox::Vector<nox::uint32> free_entities_;

		nox::Vector<nox::World::Archetype*> archetypes_;
		nox::Vector<nox::uint32> free_archetype_indices_;

		/// @brief EmptyArchetypeのバッファ
		std::array<std::byte, 256> empty_archetype_buffer_;
	};
}