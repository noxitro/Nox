// Copyright (C) 2026 NOX ENGINE All rights reserved.

/// @file	world.h
/// @brief	world
#pragma once
#include	"system.h"
#include	"entity.h"
#include	"entity_command_buffer.h"
#include	"archetype.h"
#include	"entity_system.h"
#include	"entity_logic.h"
#include	"updater_graph.h"
#include	"service.h"

namespace nox
{
	struct IComponentData;
	class Component;
	class SystemBase;
	class EngineModule;

	class World final: public nox::Object
	{
		NOX_DECLARE_OBJECT(World, nox::Object);
	private:
		static constexpr nox::uint32 k_entity_record_page_shift = 10u;
		static constexpr nox::uint32 k_entity_record_page_size = 1u << k_entity_record_page_shift;
		static constexpr nox::uint32 k_entity_record_page_mask = k_entity_record_page_size - 1u;
		static constexpr nox::uint32 k_max_entity_page_count = 1u << 14;
		static constexpr nox::uint32 k_max_entity_count = k_entity_record_page_size * k_max_entity_page_count;
		static constexpr nox::uint32 k_invalid_entity_index = std::numeric_limits<nox::uint32>::max();
		static constexpr nox::uint32 k_initial_live_generation = 1u;
		static constexpr nox::uint32 k_entity_command_capacity = 1024u;
		static constexpr nox::uint32 k_max_service_count = 64u;
		static constexpr nox::uint32 k_initial_archetype_capacity = 64u;

		/// @brief 実行ノード
		struct SystemExecuteNode
		{
			std::reference_wrapper<nox::SystemBase> instance;
			std::reference_wrapper<const nox::SystemBase::SystemPhase> phase;
			nox::uint32 layer_index;	///< 小さいほど先に実行。同一レイヤーは並列実行可能
		};

		struct EntityRecord
		{
			nox::Atomic<nox::uint32> generation;
			nox::Atomic<nox::uint32> next_free_index;
			/// @brief 所属Archetype。ComponentDataを1つも持たない間はnullptr。
			nox::Archetype* archetype;
			nox::ArchetypeLocation location;

			EntityRecord() noexcept :
				generation(0u),
				next_free_index(k_invalid_entity_index),
				archetype(nullptr),
				location(nox::ArchetypeLocation::Invalid())
			{
			}
		};

		struct ServiceEntry
		{
			const nox::reflection::Type* type;
			nox::Service* service;
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
		[[nodiscard]] bool QueueDestroyEntity(nox::EntityId entity)noexcept;
		bool IsAlive(nox::EntityId entity)const noexcept;

#pragma region ComponentData
		/// @brief ComponentDataを追加する。Archetype間の移動を伴うため列挙中・System実行中は呼べない。
		/// @return 追加された(既に持っていた場合は既存の)ComponentDataへのポインタ。
		template<class T>
			requires(nox::IsComponentDataType<T>())
		T* AddComponent(const nox::EntityId entity)
		{
			return static_cast<T*>(AddComponent(entity, nox::ComponentTypeOf<T>()));
		}

		template<class T>
			requires(nox::IsComponentDataType<T>())
		void RemoveComponent(const nox::EntityId entity)
		{
			RemoveComponent(entity, nox::ComponentTypeOf<T>());
		}

		template<class T>
			requires(nox::IsComponentDataType<T>())
		[[nodiscard]] T* TryGetComponent(const nox::EntityId entity)noexcept
		{
			return static_cast<T*>(TryGetComponent(entity, nox::ComponentTypeIndexOf<T>()));
		}

		template<class T>
			requires(nox::IsComponentDataType<T>())
		[[nodiscard]] bool HasComponent(const nox::EntityId entity)const noexcept
		{
			return HasComponent(entity, nox::ComponentTypeIndexOf<T>());
		}

		void* AddComponent(nox::EntityId entity, const nox::ComponentTypeInfo& type_info);
		void RemoveComponent(nox::EntityId entity, const nox::ComponentTypeInfo& type_info);
		[[nodiscard]] void* TryGetComponent(nox::EntityId entity, nox::ComponentTypeIndex type_index)noexcept;
		[[nodiscard]] bool HasComponent(nox::EntityId entity, nox::ComponentTypeIndex type_index)const noexcept;
#pragma endregion

#pragma region Service
		/// @brief Serviceを登録する。所有権はWorldに移り、World破棄時に解放される。
		void RegisterService(const nox::reflection::Type& type, nox::Service& service);

		template<std::derived_from<nox::Service> T>
		void RegisterService(T& service) { RegisterService(nox::reflection::Typeof<T>(), service); }

		[[nodiscard]] nox::Service* TryGetService(const nox::reflection::Type& type)const noexcept;

		template<std::derived_from<nox::Service> T>
		[[nodiscard]] T* TryGetService()const noexcept
		{
			return static_cast<T*>(TryGetService(nox::reflection::Typeof<T>()));
		}
#pragma endregion

		/// @brief 指定したComponentDataを全て持つArchetypeにマッチするQueryを構築する。
		void BuildQuery(nox::EntityQuery& query, const nox::ComponentMask& required_mask);

		/// @brief entityが所属するArchetype。ComponentDataを1つも持たない場合はnullptr。
		[[nodiscard]] nox::Archetype* TryGetArchetype(nox::EntityId entity)const noexcept;

		/// @brief entityのArchetype内での位置。所属していない場合はInvalid。
		[[nodiscard]] nox::ArchetypeLocation GetArchetypeLocation(nox::EntityId entity)const noexcept;

	private:
		void Init();
		void Update();
		void Exit();
		void BuildExecuteNodeList(std::span<nox::SystemBase*> system_list);
		void ExecutePhase(const nox::SystemPhaseType phase_type);
		void RegisterSystem(nox::SystemBase& system);
		void FlushEntityCommands()noexcept;
		void DestroyEntityImmediate(nox::EntityId entity)noexcept;

		void CreateEntitySystems();
		void CreateEntityLogicStorages();
		/// @brief UpdaterGraphのレイヤー順にノードを実行する。
		/// @details 現段階は「レイヤー順・レイヤー内は登録順」の直列実行。
		///          同一レイヤーのノードは互いに衝突しないため、stage 2bではこのループが配分点になる。
		void ExecuteUpdaterGraphPhase(nox::SystemPhaseType phase_type);
		/// @brief ノード1つを実行する。stage 2bではこの関数をそのままワーカーへ渡す。
		void ExecuteNode(const nox::UpdaterNode& node);
		/// @brief entityのComponentData構成が変わったので、EntityLogicの生成/破棄を追従させる。
		void RefreshEntityLogics(nox::EntityId entity, const nox::Archetype* archetype);

		[[nodiscard]] nox::Archetype& GetOrCreateArchetype(const nox::ComponentMask& mask);
		[[nodiscard]] nox::Archetype* TryFindArchetype(const nox::ComponentMask& mask)const noexcept;
		/// @brief entityを別のArchetypeへ移す。共通のComponentDataだけが引き継がれる。
		void MoveEntityToArchetype(nox::World::EntityRecord& entity_record, nox::EntityId entity, nox::Archetype* destination);
		/// @brief swap-removeで移動してきたentityの位置情報を更新する。
		void PatchMovedEntityLocation(nox::EntityId moved_entity, nox::ArchetypeLocation location)noexcept;

#if !NOX_MASTER
		void TraceExecuteNodeList()const;

		/// @brief ノードが宣言したComponentData / Serviceの並列実行チェックに入る。
		void EnterNodeAccessScope(const nox::UpdaterNodeAccess& access)noexcept;
		/// @brief EnterNodeAccessScopeで入ったチェックから抜ける。
		void LeaveNodeAccessScope(const nox::UpdaterNodeAccess& access)noexcept;
		/// @brief Serviceの型に対応するチェッカー。未登録のServiceならnullptr。
		[[nodiscard]] nox::util::RWParallelExecuteChecker* TryGetServiceExecuteChecker(const nox::reflection::Type* type)noexcept;
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
		std::atomic_bool is_executing_system_phase_;
		nox::EntityCommandBuffer<k_entity_command_capacity> entity_command_buffer_;

		NOX_ATTR(nox::reflection::attr::IgnoreReflection())
		nox::Vector<nox::Archetype*> archetypes_;

		NOX_ATTR(nox::reflection::attr::IgnoreReflection())
		nox::Vector<nox::EntitySystemBase*> entity_systems_;

		NOX_ATTR(nox::reflection::attr::IgnoreReflection())
		nox::Vector<nox::EntityLogicStorage*> entity_logic_storages_;

		NOX_ATTR(nox::reflection::attr::IgnoreReflection())
		nox::UpdaterGraph updater_graph_;

#if !NOX_MASTER
		//	依存解析の誤りを即座に検出するためのチェッカー。ComponentTypeIndexごと / Service登録順ごとに1つ持つ。
		//	直列実行の現段階では決して発火しない。stage 2bで並列化したときに、宣言と実アクセスの
		//	食い違いをその場で落とすための土台。
		NOX_ATTR(nox::reflection::attr::IgnoreReflection())
		std::array<nox::util::RWParallelExecuteChecker, k_max_component_type_count> component_execute_checkers_;

		NOX_ATTR(nox::reflection::attr::IgnoreReflection())
		std::array<nox::util::RWParallelExecuteChecker, k_max_service_count> service_execute_checkers_;
#endif // !NOX_MASTER

		NOX_ATTR(nox::reflection::attr::IgnoreReflection())
		nox::FixedVector<nox::World::ServiceEntry, k_max_service_count> services_;

		nox::Vector<nox::EngineModule*> modules_;
		nox::Vector<nox::SystemBase*> systems_;
		nox::UnorderedMap<const nox::reflection::Type*, nox::SystemBase*> system_map_;
		std::array<nox::Vector<SystemExecuteNode>, nox::util::ToUnderlying(nox::SystemPhaseType::_Max)> system_phase_table_;
	};
}