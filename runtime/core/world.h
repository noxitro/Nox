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

	class World final: public nox::Object
	{
		NOX_DECLARE_OBJECT(World, nox::Object);
	public:
		struct Archtype {};
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
		~World()override;

		void Run();
		inline constexpr nox::uint32 GetFrameCount()const noexcept { return frame_counter_; }
		inline constexpr nox::uint16 GetTargetFrameRate()const noexcept { return target_frame_rate_; }
		inline constexpr bool EnabledVSync()const noexcept { return enabled_vsync_; }
		void SetVSync(bool flag)noexcept;

		inline bool IsKill()const noexcept { return kill_.load(std::memory_order_acquire); }
		inline bool IsStudioMode()const noexcept { return studio_mode_; }

		nox::SystemBase* FindSystem(const nox::reflection::Type& type)const noexcept;

		template<std::derived_from<nox::SystemBase> T>
		inline T* FindSystem()const noexcept
		{
			return static_cast<T*>(FindSystem(nox::reflection::Typeof<T>()));
		}

		nox::SystemBase& GetSystem(const nox::reflection::Type& type)const;

		template<std::derived_from<nox::SystemBase> T>
		inline T& GetSystem()const
		{
			return static_cast<T&>(GetSystem(nox::reflection::Typeof<T>()));
		}

#if !NOX_MASTER
		NOX_ATTR_DECLARE(::nox::reflection::attr::IgnoreReflection())
		nox::U8FixedString<3072> BuildRuntimeDependencyGraphText()const;
#endif // !NOX_MASTER

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
		void Exit();
		void BuildExecuteNodeList(std::span<nox::SystemBase*> system_list);
		void ExecutePhase(const nox::SystemPhaseType phase_type);
		void RegisterSystem(nox::SystemBase& system);

#if !NOX_MASTER
		void TraceExecuteNodeList()const;
#endif // !NOX_MASTER

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

		nox::StopWatch stop_watch_;
		nox::uint32 frame_counter_;
		nox::float_t elapsed_milli_seconds_;
		nox::float_t next_elapsed_milli_seconds_;
		nox::uint16 target_frame_rate_;
		bool enabled_vsync_;
		const bool studio_mode_;

		NOX_ATTR(nox::reflection::attr::IgnoreReflection())
		std::atomic_bool kill_;

		nox::Vector<nox::EngineModule*> modules_;
		nox::Vector<nox::SystemBase*> systems_;
		nox::UnorderedMap<const nox::reflection::Type*, nox::SystemBase*> system_map_;
		std::array<nox::Vector<ExecuteNode>, nox::util::ToUnderlying(nox::SystemPhaseType::_Max)> system_phase_table_;
	};
}