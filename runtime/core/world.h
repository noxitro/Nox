// Copyright (C) 2026 NOX ENGINE All rights reserved.

/// @file	world.h
/// @brief	world
#pragma once
#include	"system.h"
#include	"entity.h"

namespace nox
{
	struct IComponentData;
	class Component;
	class SystemBase;
	class EngineModule;

	class World : public nox::Object
	{
		NOX_DECLARE_OBJECT(World, nox::Object);
	public:
		struct Archtype;
	private:
		static constexpr nox::uint32 k_entity_record_page_shift = 10u;
		static constexpr nox::uint32 k_entity_record_page_size = 1u << k_entity_record_page_shift;
		static constexpr nox::uint32 k_entity_record_page_mask = k_entity_record_page_size - 1u;
		static constexpr nox::uint32 k_max_entity_page_count = 1u << 14;
		static constexpr nox::uint32 k_max_entity_count = k_entity_record_page_size * k_max_entity_page_count;
		static constexpr nox::uint32 k_invalid_entity_index = std::numeric_limits<nox::uint32>::max();
		static constexpr nox::uint32 k_initial_live_generation = 1u;

		/// @brief 実行ノード
		struct ExecuteNode
		{
			std::reference_wrapper<nox::SystemBase> instance;
			std::reference_wrapper<const nox::SystemBase::SystemPhase> phase;
			nox::uint32 layer_index;	///< 小さいほど先に実行。同一レイヤーは並列実行可能
		};

		struct EntityRecord
		{
			nox::Atomic<nox::uint32> generation;
			nox::Atomic<nox::uint32> next_free_index;
			nox::uint32 row;
			nox::World::Archtype* archtype;

			EntityRecord() noexcept :
				generation(0u),
				next_free_index(k_invalid_entity_index),
				row(0u),
				archtype(nullptr)
			{
			}
		};

		struct EntityRecordPage
		{
			std::array<nox::World::EntityRecord, k_entity_record_page_size> records;
		};
	public:
		World();
		~World();

		void Run();

		nox::EntityId CreateEntity();
		void DestroyEntity(nox::EntityId entity);
		bool IsAlive(nox::EntityId entity)const noexcept;

		nox::IComponentData* CreateComponent(nox::EntityId entity, const nox::reflection::Type& type);

		template<std::derived_from<nox::IComponentData> T>
		T* CreateComponent(nox::EntityId entity)
		{
			return static_cast<T*>(CreateComponent(entity, nox::reflection::Typeof<T>()));
		}

		template<class _F>
		void Each(_F&& func)
		{

		}

		template<class T>
		void SetResource() {}

		template<class T>
		T* TryGetResource()const { return nullptr; }


	private:
		void Init();
		void Update();
		void Terminate();

		[[nodiscard]]
		nox::World::EntityRecord* TryGetEntityRecord(nox::uint32 index) noexcept;

		[[nodiscard]]
		const nox::World::EntityRecord* TryGetEntityRecord(nox::uint32 index) const noexcept;

		[[nodiscard]]
		nox::World::EntityRecord* EnsureEntityRecord(nox::uint32 index);

		[[nodiscard]]
		nox::uint32 TryPopFreeEntityIndex() noexcept;

		void PushFreeEntityIndex(nox::uint32 index) noexcept;

	private:
		NOX_ATTR(nox::reflection::attr::IgnoreReflection())
		nox::Atomic<nox::uint64> free_entity_head_;

		NOX_ATTR(nox::reflection::attr::IgnoreReflection())
		nox::Atomic<nox::uint32> next_entity_index_;

		NOX_ATTR(nox::reflection::attr::IgnoreReflection())
		nox::World::EntityRecordPage first_entity_record_page_;

		NOX_ATTR(nox::reflection::attr::IgnoreReflection())
		std::array<nox::Atomic<nox::World::EntityRecordPage*>, k_max_entity_page_count> entity_record_pages_;

		NOX_ATTR(nox::reflection::attr::IgnoreReflection())
		std::mutex entity_record_page_mutex_;

		nox::Vector<nox::EngineModule*> modules_;
		nox::Vector<nox::SystemBase*> systems_;

		const bool studio_mode_;
		bool kill_;
		bool enabled_vsync_;
	};
}