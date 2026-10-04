//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	world.h
///	@brief	world
#pragma once
#include	"entity.h"

namespace nox
{
	class World final
	{
	private:

		struct EntityRecord final
		{
			constexpr EntityRecord()noexcept : generation(0u) {}

			nox::uint32 generation;
		};

		struct Archetype final
		{

		};
	public:
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
		nox::Vector<EntityRecord> entity_records_;
		nox::Vector<nox::uint32> free_entities_;

		
	};
}