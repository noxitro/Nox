// Copyright (C) 2026 NOX ENGINE All rights reserved.

/// @file	entity_ecs_test.cpp
/// @brief	Archetypeストレージ / シグネチャ解析 / EntitySystem / EntityLogic のテスト。
#include "pch.h"
#include "test.h"

#include "entity_ecs_test.h"
#include "../world.h"
#include "../entity_system.h"
#include "../entity_logic.h"
#include "../entity_type_registry.h"
#include "../updater_graph.h"
#include "../log_id.h"
#include "../../kernel/assertion.h"
#include "../../kernel/job_system.h"

namespace nox::test::ecs::manual
{
	/// @brief 生成器が名前を書けない型のための手動購読経路の検証用。
	/// @details .cppに閉じているためリフレクション生成コードからは見えない。
	///          nox::EntityLogicMethodTableの特殊化を手書きすれば同じ記述子が作れる。
	class ManualHealthLogic final : public nox::EntityLogic<nox::test::ecs::manual::ManualHealthLogic>
	{
	public:
		void Tick(nox::test::ecs::TestHealth& health)
		{
			++tick_count;
			--health.value;
		}

		nox::int32 tick_count = 0;
	};
}

//	明示的特殊化はnoxを囲む名前空間に書く必要があるため、グローバルスコープに置く。
template<>
struct nox::EntityLogicMethodTable<nox::test::ecs::manual::ManualHealthLogic>
{
	static constexpr nox::EntityLogicMethodDescriptor k_methods[]{
		nox::MakeEntityLogicMethodDescriptor<
			&nox::test::ecs::manual::ManualHealthLogic::Tick,
			nox::SystemPhaseType::Update>("Tick"),
	};

	[[nodiscard]] static constexpr std::span<const nox::EntityLogicMethodDescriptor> GetMethods()noexcept
	{
		return std::span<const nox::EntityLogicMethodDescriptor>(k_methods);
	}
};

namespace
{
	using nox::test::ecs::TestCounterService;
	using nox::test::ecs::TestHealth;
	using nox::test::ecs::TestPosition;
	using nox::test::ecs::TestVelocity;
	using nox::test::ecs::TestMoveSystem;
	using nox::test::ecs::TestParallelAddSystem;
	using nox::test::ecs::TestPlayerLogic;

	//	Chunk並列の宣言はオプトイン。宣言していない型は既定でfalseのままでなければならない。
	static_assert(nox::IsParallelForEachEntitySystem<TestParallelAddSystem>());
	static_assert(nox::IsParallelForEachEntitySystem<TestMoveSystem>() == false);
	static_assert(nox::k_entity_system_type_descriptor<TestParallelAddSystem>.parallel_for_each);
	static_assert(nox::k_entity_system_type_descriptor<TestMoveSystem>.parallel_for_each == false);

#pragma region シグネチャ解析のコンパイル時検証

	using MoveSignature = nox::EntitySignature<nox::EntityId, TestPosition&, const TestVelocity&>;
	using ReadOnlySignature = nox::EntitySignature<const TestPosition&, const TestVelocity&>;
	using HealthSignature = nox::EntitySignature<TestHealth&>;

	static_assert(MoveSignature::k_is_valid);
	static_assert(MoveSignature::k_parameter_count == 3u);
	static_assert(MoveSignature::k_component_parameter_count == 2u);
	static_assert(MoveSignature::k_entity_parameter_count == 1u);

	//	const参照は読み取りのみ、非const参照は読み書き。
	static_assert(MoveSignature::HasWriteAccess<TestPosition>);
	static_assert(MoveSignature::HasReadAccess<TestVelocity>);
	static_assert(MoveSignature::HasWriteAccess<TestVelocity> == false);
	static_assert(MoveSignature::HasReadAccess<TestHealth> == false);

	//	同一ComponentDataにRWが絡めば直列化、全てROなら並列。
	static_assert(nox::CanRunConcurrently<ReadOnlySignature, ReadOnlySignature>);
	static_assert(nox::CanRunConcurrently<MoveSignature, ReadOnlySignature> == false);
	static_assert(nox::CanRunConcurrently<MoveSignature, HealthSignature>);

	//	EntityIdは先頭にのみ1つ。ComponentDataの重複宣言も不可。
	static_assert(nox::EntitySignature<TestPosition&, nox::EntityId>::k_is_valid == false);
	static_assert(nox::EntitySignature<nox::EntityId, nox::EntityId>::k_is_valid == false);
	static_assert(nox::EntitySignature<TestPosition&, const TestPosition&>::k_is_valid == false);
	//	ComponentDataでもServiceでもない型は引数にできない。
	static_assert(nox::EntitySignature<nox::int32&>::k_all_parameters_valid == false);
	//	ComponentDataはポインタでなく参照で受ける(値渡し・ポインタ渡しは宣言として認めない)。
	static_assert(nox::EntitySignature<TestPosition*>::k_all_parameters_valid == false);
	static_assert(nox::EntitySignature<TestPosition>::k_all_parameters_valid == false);
	//	Serviceはポインタでも参照でも受けられる。constの有無が読み書き権限になるのはComponentDataと同じ。
	static_assert(nox::EntitySignature<TestCounterService*>::k_is_valid);
	static_assert(nox::EntitySignature<const TestCounterService*>::k_is_valid);
	static_assert(nox::EntitySignature<TestCounterService&>::k_is_valid);
	static_assert(nox::EntitySignature<const TestCounterService&>::k_is_valid);
	static_assert(nox::EntityParameterTraits<TestCounterService&>::k_kind == nox::EntityParameterKind::ServiceWrite);
	static_assert(nox::EntityParameterTraits<const TestCounterService&>::k_kind == nox::EntityParameterKind::ServiceRead);
	//	Serviceは引数に並べてもComponentDataの宣言には算入されない(Queryの必須条件を変えない)。
	static_assert(nox::EntitySignature<TestPosition&, TestCounterService&>::k_component_parameter_count == 1u);
	static_assert(nox::EntitySignature<TestPosition&, TestCounterService&>::k_service_parameter_count == 1u);

	//	nox::EntityCommandsは引数として並べられるが、ComponentDataにもServiceにも算入されない。
	static_assert(nox::EntitySignature<nox::EntityId, TestPosition&, nox::EntityCommands&>::k_is_valid);
	static_assert(nox::EntitySignature<nox::EntityCommands&>::k_component_parameter_count == 0u);
	static_assert(nox::EntitySignature<nox::EntityCommands&>::k_service_parameter_count == 0u);
	//	constではDestroyを呼べず宣言として意味を成さないため、const参照では受けられない。
	static_assert(nox::EntitySignature<const nox::EntityCommands&>::k_all_parameters_valid == false);

	//	EntityLogicの基底はentityだけを持つ(Worldへの参照は保持しない)。
	static_assert(sizeof(nox::EntityLogic<TestPlayerLogic>) == 8u);

	//	引数リストがそのままメソッドの宣言として解釈される。
	struct SignatureProbe
	{
		void Process00(nox::EntityId entity, TestPosition& position, TestCounterService* service);
	};
	static_assert(nox::EntityMethod<decltype(&SignatureProbe::Process00)>);
	static_assert(std::same_as<
		nox::EntityMethodTraits<decltype(&SignatureProbe::Process00)>::Signature,
		nox::EntitySignature<nox::EntityId, TestPosition&, TestCounterService*>>);

	//	const / noexcept は付けられる。const性は k_is_const に出る。
	struct QualifiedProbe
	{
		void Plain(TestPosition& position);
		void Const(const TestPosition& position)const;
		void Noexcept(TestPosition& position)noexcept;
		void ConstNoexcept(const TestPosition& position)const noexcept;
	};
	static_assert(nox::EntityMethod<decltype(&QualifiedProbe::Plain)>);
	static_assert(nox::EntityMethod<decltype(&QualifiedProbe::Const)>);
	static_assert(nox::EntityMethod<decltype(&QualifiedProbe::Noexcept)>);
	static_assert(nox::EntityMethod<decltype(&QualifiedProbe::ConstNoexcept)>);
	static_assert(nox::EntityMethodTraits<decltype(&QualifiedProbe::Plain)>::k_is_const == false);
	static_assert(nox::EntityMethodTraits<decltype(&QualifiedProbe::Const)>::k_is_const);
	static_assert(nox::EntityMethodTraits<decltype(&QualifiedProbe::ConstNoexcept)>::k_is_const);
	static_assert(std::same_as<
		nox::EntityMethodTraits<decltype(&QualifiedProbe::ConstNoexcept)>::OwnerType, QualifiedProbe>);

	//	更新メソッドにできない形。concept評価がハードエラーにならずfalseになること。
	struct RejectedProbe
	{
		int NonVoidReturn(TestPosition& position);
		void Volatile(TestPosition& position)volatile;
		void LValueRefQualified(TestPosition& position)&;
		void RValueRefQualified(TestPosition& position)&&;
	};
	static_assert(nox::EntityMethod<decltype(&RejectedProbe::NonVoidReturn)> == false);
	static_assert(nox::EntityMethod<decltype(&RejectedProbe::Volatile)> == false);
	static_assert(nox::EntityMethod<decltype(&RejectedProbe::LValueRefQualified)> == false);
	static_assert(nox::EntityMethod<decltype(&RejectedProbe::RValueRefQualified)> == false);
	//	メンバ関数ポインタ以外を渡してもハードエラーにしない。
	static_assert(nox::EntityMethod<nox::int32> == false);
	static_assert(nox::EntityMethod<void(*)(TestPosition&)> == false);

#pragma endregion

	[[nodiscard]] const nox::EntitySystemTypeDescriptor* FindEntitySystemType(const std::string_view name)noexcept
	{
		for (const nox::EntitySystemTypeDescriptor* const descriptor : nox::GetEntitySystemTypes())
		{
			if (descriptor->name.find(name) != std::string_view::npos)
			{
				return descriptor;
			}
		}
		return nullptr;
	}

	[[nodiscard]] const nox::EntityLogicTypeDescriptor* FindEntityLogicType(const std::string_view name)noexcept
	{
		for (const nox::EntityLogicTypeDescriptor* const descriptor : nox::GetEntityLogicTypes())
		{
			if (descriptor->name.find(name) != std::string_view::npos)
			{
				return descriptor;
			}
		}
		return nullptr;
	}

	void TestComponentTypeRegistry()
	{
		//	型ごとに一意な密インデックスが振られる。
		NOX_ASSERT(nox::ComponentTypeIndexOf<TestPosition>() != nox::ComponentTypeIndexOf<TestVelocity>(),
			u"ComponentTypeIndexが重複しています");
		NOX_ASSERT(nox::ComponentTypeOf<TestPosition>().size == sizeof(TestPosition),
			u"ComponentTypeInfoのサイズが不正です");

		const nox::ComponentMask mask = nox::MakeComponentMask<TestPosition, TestVelocity>();
		NOX_ASSERT(mask.Test(nox::ComponentTypeIndexOf<TestPosition>()), u"ComponentMaskにビットが立っていません");
		NOX_ASSERT(mask.Test(nox::ComponentTypeIndexOf<TestHealth>()) == false, u"ComponentMaskに余分なビットが立っています");
		NOX_ASSERT(mask.Contains(nox::MakeComponentMask<TestPosition>()), u"ComponentMask::Containsが不正です");
		NOX_ASSERT(nox::MakeComponentMask<TestPosition>().Contains(mask) == false, u"ComponentMask::Containsが不正です");
	}

	void TestArchetypeStorage()
	{
		const std::array<const nox::ComponentTypeInfo*, 2> types{
			&nox::ComponentTypeOf<TestPosition>(),
			&nox::ComponentTypeOf<TestVelocity>(),
		};
		nox::Archetype archetype(nox::MakeComponentMask<TestPosition, TestVelocity>(), std::span(types));

		NOX_ASSERT(archetype.GetChunkCapacity() > 0u, u"Chunkの容量が0です");

		//	Chunkをまたぐ数だけ詰めて、位置とデータの対応が崩れないことを見る。
		const nox::uint32 entity_count = archetype.GetChunkCapacity() + 3u;
		nox::Vector<nox::ArchetypeLocation> locations;
		for (nox::uint32 entity_index = 0u; entity_index < entity_count; ++entity_index)
		{
			const nox::EntityId entity{ .generation = 1u, .index = entity_index };
			const nox::ArchetypeLocation location = archetype.AddEntity(entity);
			locations.push_back(location);

			auto* const positions = static_cast<TestPosition*>(
				archetype.TryGetComponentArray(location.chunk_index, nox::ComponentTypeIndexOf<TestPosition>()));
			NOX_ASSERT(positions != nullptr, u"ComponentDataの列が取得できません");
			positions[location.row].x = static_cast<nox::float32>(entity_index);
		}

		NOX_ASSERT(archetype.GetEntityCount() == entity_count, u"Archetypeのentity数が不正です");
		NOX_ASSERT(archetype.GetChunkCount() >= 2u, u"Chunkが分割されていません");

		for (nox::uint32 entity_index = 0u; entity_index < entity_count; ++entity_index)
		{
			const nox::ArchetypeLocation& location = locations[entity_index];
			const auto* const positions = static_cast<const TestPosition*>(
				archetype.TryGetComponentArray(location.chunk_index, nox::ComponentTypeIndexOf<TestPosition>()));
			NOX_ASSERT(positions[location.row].x == static_cast<nox::float32>(entity_index),
				u"Chunkをまたいだ書き込み位置がずれています");
		}

		//	swap-removeで末尾のentityが穴に移動してくる。
		const nox::EntityId moved = archetype.RemoveEntity(locations[0]);
		NOX_ASSERT(moved.index == entity_count - 1u, u"swap-removeで移動したentityが不正です");
		const auto* const positions = static_cast<const TestPosition*>(
			archetype.TryGetComponentArray(locations[0].chunk_index, nox::ComponentTypeIndexOf<TestPosition>()));
		NOX_ASSERT(positions[locations[0].row].x == static_cast<nox::float32>(entity_count - 1u),
			u"swap-removeでComponentDataが移動していません");

		//	宣言外の型は引けない。
		NOX_ASSERT(archetype.TryGetComponentArray(0u, nox::ComponentTypeIndexOf<TestHealth>()) == nullptr,
			u"Archetypeが持たない型の列が引けてしまいました");
	}

	void TestWorldStructuralChange(nox::World& world)
	{
		const nox::EntityId entity = world.CreateEntity();
		NOX_ASSERT(world.IsAlive(entity), u"生成直後のentityが生存していません");
		NOX_ASSERT(world.HasComponent<TestPosition>(entity) == false, u"未追加のComponentDataを持っています");

		auto* const position = world.AddComponent<TestPosition>(entity);
		NOX_ASSERT(position != nullptr, u"ComponentDataの追加に失敗しました");
		position->x = 10.0f;
		position->y = 20.0f;

		//	Archetype間の移動を挟んでも既存のComponentDataは引き継がれる。
		auto* const velocity = world.AddComponent<TestVelocity>(entity);
		NOX_ASSERT(velocity != nullptr, u"ComponentDataの追加に失敗しました");
		velocity->x = 1.0f;

		const auto* const moved_position = world.TryGetComponent<TestPosition>(entity);
		NOX_ASSERT(moved_position != nullptr, u"Archetype移動後にComponentDataが引けません");
		NOX_ASSERT(moved_position->x == 10.0f && moved_position->y == 20.0f,
			u"Archetype移動でComponentDataの値が失われました");

		//	削除しても残りは保持される。
		world.RemoveComponent<TestVelocity>(entity);
		NOX_ASSERT(world.HasComponent<TestVelocity>(entity) == false, u"ComponentDataが削除されていません");
		NOX_ASSERT(world.TryGetComponent<TestPosition>(entity)->x == 10.0f,
			u"ComponentData削除で他の値が失われました");

		//	同じentityに2つ目を足してswap-removeを起こしても、他entityの位置情報が壊れない。
		const nox::EntityId other = world.CreateEntity();
		world.AddComponent<TestPosition>(other)->x = 99.0f;
		world.DestroyEntity(entity);
		NOX_ASSERT(world.IsAlive(entity) == false, u"破棄したentityが生存しています");
		NOX_ASSERT(world.TryGetComponent<TestPosition>(other) != nullptr,
			u"swap-remove後に他entityのComponentDataが引けません");
		NOX_ASSERT(world.TryGetComponent<TestPosition>(other)->x == 99.0f,
			u"swap-remove後に他entityの位置情報が更新されていません");

		world.DestroyEntity(other);
	}

	void TestEntitySystemExecution(nox::World& world)
	{
		//	ヘッダに定義しただけで、生成コードの表に型記述子が載っている。
		const nox::EntitySystemTypeDescriptor* const registered = FindEntitySystemType("TestMoveSystem");
		NOX_ASSERT(registered != nullptr, u"EntitySystemが自動登録されていません");
		NOX_ASSERT(registered != nullptr && registered->execute != nullptr, u"EntitySystemの実行本体が束縛されていません");

		nox::FixedVector<nox::EntityId, 4> entities;
		for (nox::uint32 entity_index = 0u; entity_index < 4u; ++entity_index)
		{
			const nox::EntityId entity = world.CreateEntity();
			entities.PushBack(entity);
			world.AddComponent<TestPosition>(entity)->x = static_cast<nox::float32>(entity_index);
			world.AddComponent<TestVelocity>(entity)->x = 2.0f;
		}

		//	宣言したComponentDataを持たないentityは走査対象に入らない。
		const nox::EntityId ignored = world.CreateEntity();
		world.AddComponent<TestPosition>(ignored)->x = 1000.0f;

		TestMoveSystem system;
		world.BuildQuery(system.GetQuery(), system.GetDescriptor().make_read_write_mask());
		system.Execute(world);

		NOX_ASSERT(system.processed_count == 4, u"EntitySystemの処理対象数が不正です");
		for (nox::uint32 entity_index = 0u; entity_index < 4u; ++entity_index)
		{
			const nox::EntityId entity = entities.GetStorage()[entity_index];
			NOX_ASSERT(world.TryGetComponent<TestPosition>(entity)->x == static_cast<nox::float32>(entity_index) + 2.0f,
				u"EntitySystemの書き込み結果が不正です");
		}
		NOX_ASSERT(world.TryGetComponent<TestPosition>(ignored)->x == 1000.0f,
			u"宣言外のentityが処理されました");

		for (nox::uint32 entity_index = 0u; entity_index < 4u; ++entity_index)
		{
			world.DestroyEntity(entities.GetStorage()[entity_index]);
		}
		world.DestroyEntity(ignored);
	}

	/// @brief Worldがstage 2cで通る経路をそのまま組み立ててSystemを1回走らせる。
	/// @details nox::World::ExecuteEntitySystemParallel と同じ部品(Queryのchunk列挙 → 記述子の
	///          execute_chunk → JobSystemのDispatch/Wait)を、テストからワーカー数を切り替えられる形で並べたもの。
	///          Worldのjob_system_はInit()でしか起動されないため、テストは自前のJobSystemを持つ。
	void ExecuteEntitySystemChunkParallel(
		nox::World& world,
		nox::EntitySystemBase& system,
		nox::JobSystem& job_system)
	{
		static constexpr nox::uint32 k_max_chunk_jobs = 256u;

		struct ChunkJobContext
		{
			nox::World* world;
			nox::EntitySystemBase* system;
			nox::Archetype* archetype;
			nox::uint32 chunk_index;
		};

		const nox::EntityQuery& query = system.GetQuery();
		const nox::uint32 total_chunk_count = query.GetTotalChunkCount();
		if (total_chunk_count <= 1u || job_system.GetWorkerCount() == 0u)
		{
			system.Execute(world);
			return;
		}

		std::array<nox::EntityChunkRef, k_max_chunk_jobs> chunk_refs{};
		std::array<ChunkJobContext, k_max_chunk_jobs> contexts{};
		std::array<nox::Job, k_max_chunk_jobs> jobs{};

		for (nox::uint32 start = 0u; start < total_chunk_count; start += k_max_chunk_jobs)
		{
			const nox::uint32 job_count = query.FillChunkRefs(start, std::span<nox::EntityChunkRef>(chunk_refs));
			if (job_count == 0u)
			{
				break;
			}

			for (nox::uint32 index = 0u; index < job_count; ++index)
			{
				contexts[index] = ChunkJobContext{
					.world = &world,
					.system = &system,
					.archetype = chunk_refs[index].archetype,
					.chunk_index = chunk_refs[index].chunk_index,
				};
				jobs[index] = nox::Job{
					.func = [](void* const context)
						{
							auto* const job_context = static_cast<ChunkJobContext*>(context);
							job_context->system->ExecuteChunk(
								*job_context->world, *job_context->archetype, job_context->chunk_index);
						},
					.context = &contexts[index],
				};
			}

			nox::JobCounter counter{ 0u };
			job_system.Dispatch(std::span<const nox::Job>(jobs.data(), job_count), counter);
			job_system.Wait(counter);
		}
	}

	/// @brief 複数Chunkにまたがるentity群を、ワーカー0本 / 既定本数の双方で処理して結果が一致することを見る。
	void TestParallelForEachEntitySystem(nox::World& world)
	{
		const nox::EntitySystemTypeDescriptor* const registered = FindEntitySystemType("TestParallelAddSystem");
		NOX_ASSERT(registered != nullptr, u"Chunk並列SystemがWorldの表に載っていません");
		NOX_ASSERT(registered != nullptr && registered->parallel_for_each,
			u"k_parallel_for_eachの宣言が記述子に伝わっていません");
		NOX_ASSERT(registered != nullptr && registered->execute_chunk != nullptr,
			u"Chunk単位の実行本体が束縛されていません");

		//	Chunk容量はArchetypeが決めるので、まず1つ作って容量を読む。
		const nox::EntityId probe = world.CreateEntity();
		world.AddComponent<TestPosition>(probe)->x = 0.0f;
		world.AddComponent<TestVelocity>(probe)->x = 1.0f;

		nox::Archetype* const archetype = world.TryGetArchetype(probe);
		NOX_ASSERT(archetype != nullptr, u"Archetypeが引けません");
		if (archetype == nullptr)
		{
			world.DestroyEntity(probe);
			return;
		}

		//	1Chunkに収まらない数を作る。複数Chunkに割れていることは後段でGetChunkCountを見て確認する。
		const nox::uint32 entity_count = archetype->GetChunkCapacity() + 5u;

		nox::Vector<nox::EntityId> entities;
		entities.reserve(entity_count);
		entities.push_back(probe);
		for (nox::uint32 index = 1u; index < entity_count; ++index)
		{
			const nox::EntityId entity = world.CreateEntity();
			entities.push_back(entity);
			world.AddComponent<TestPosition>(entity)->x = 0.0f;
			world.AddComponent<TestVelocity>(entity)->x = 1.0f;
		}

		TestParallelAddSystem system;
		world.BuildQuery(system.GetQuery(), system.GetDescriptor().make_read_write_mask());

		const nox::uint32 chunk_count = system.GetQuery().GetTotalChunkCount();
		NOX_ASSERT(chunk_count >= 2u, u"複数Chunkにまたがっていません(テストの前提が崩れています)");

		//	chunk参照の列挙は「空でないChunkをちょうど1回ずつ」でなければならない。
		std::array<nox::EntityChunkRef, 64> refs{};
		const nox::uint32 filled = system.GetQuery().FillChunkRefs(0u, std::span<nox::EntityChunkRef>(refs));
		NOX_ASSERT(filled == chunk_count, u"chunk参照の列挙数が総数と一致しません");
		for (nox::uint32 i = 0u; i < filled; ++i)
		{
			for (nox::uint32 j = i + 1u; j < filled; ++j)
			{
				NOX_ASSERT(
					(refs[i].archetype != refs[j].archetype) || (refs[i].chunk_index != refs[j].chunk_index),
					u"同じChunkが2回列挙されています(二重更新になります)");
			}
		}

		//	ワーカー0本(--serial-updater相当)。この場合はDispatchが呼び出しスレッドで全部回す。
		{
			nox::JobSystem serial_job_system;
			serial_job_system.Initialize(0u);
			NOX_ASSERT(serial_job_system.GetWorkerCount() == 0u, u"ワーカー0本の指定が効いていません");
			ExecuteEntitySystemChunkParallel(world, system, serial_job_system);
			serial_job_system.Finalize();
		}

		for (nox::uint32 index = 0u; index < entity_count; ++index)
		{
			NOX_ASSERT(world.TryGetComponent<TestPosition>(entities[index])->x == 1.0f,
				u"ワーカー0本のChunk実行で、全entityがちょうど1回だけ更新されていません");
		}

		//	既定ワーカー数。Chunkがワーカーへ配られる。結果は0本のときと完全に一致しなければならない。
		{
			nox::JobSystem parallel_job_system;
			parallel_job_system.Initialize(nox::JobSystem::GetDefaultWorkerCount());
			ExecuteEntitySystemChunkParallel(world, system, parallel_job_system);
			parallel_job_system.Finalize();
		}

		for (nox::uint32 index = 0u; index < entity_count; ++index)
		{
			NOX_ASSERT(world.TryGetComponent<TestPosition>(entities[index])->x == 2.0f,
				u"並列Chunk実行で、全entityがちょうど1回だけ更新されていません");
		}

		NOX_INFO_LINE(nox::log_id::CoreCommon,
			u8"Chunk並列テスト: entity={0} chunk={1} chunk容量={2} ワーカー={3}",
			entity_count, chunk_count, archetype->GetChunkCapacity(), nox::JobSystem::GetDefaultWorkerCount());

		for (nox::uint32 index = 0u; index < entity_count; ++index)
		{
			world.DestroyEntity(entities[index]);
		}
	}

	void TestEntityLogicLifecycle(nox::World& world)
	{
		const nox::EntityLogicTypeDescriptor* const logic_descriptor = FindEntityLogicType("TestPlayerLogic");
		NOX_ASSERT(logic_descriptor != nullptr, u"EntityLogicが自動登録されていません");
		if (logic_descriptor == nullptr)
		{
			return;
		}

		//	必須ComponentDataは全メソッドの引数の和集合になる。
		//	Process2だけが宣言しているTestVelocityも含まれ、Service / EntityCommands は含まれない。
		const nox::ComponentMask required_mask = logic_descriptor->make_required_mask();
		const nox::ComponentMask expected_mask = nox::MakeComponentMask<TestPosition, TestHealth, TestVelocity>();
		NOX_ASSERT(required_mask == expected_mask,
			u"必須ComponentDataがメソッドの引数から導出されていません");

		auto* const counter_service = new TestCounterService();
		world.RegisterService(*counter_service);
		NOX_ASSERT(world.TryGetService<TestCounterService>() == counter_service, u"Serviceが引けません");

		nox::EntityLogicStorage storage(*logic_descriptor);
		const nox::EntityId entity = world.CreateEntity();

		//	必須ComponentDataが揃うまでインスタンスは作られない。
		world.AddComponent<TestPosition>(entity)->x = 5.0f;
		world.AddComponent<TestHealth>(entity)->value = 3;
		NOX_ASSERT(world.TryGetArchetype(entity)->GetMask().Contains(required_mask) == false,
			u"必須ComponentDataがまだ揃っていないはずです");

		world.AddComponent<TestVelocity>(entity)->x = 2.0f;
		NOX_ASSERT(world.TryGetArchetype(entity)->GetMask().Contains(required_mask),
			u"必須ComponentDataが揃っていません");

		storage.CreateInstance(world, entity);
		NOX_ASSERT(storage.Contains(entity), u"EntityLogicのインスタンスが生成されていません");
		NOX_ASSERT(storage.GetEntries().size() == 1u, u"EntityLogicのインスタンス数が不正です");

		//	メソッド呼び出しは引数リストどおりに束縛される。
		for (const nox::EntityLogicMethodDescriptor& method : logic_descriptor->get_methods())
		{
			for (const nox::EntityLogicStorage::Entry& entry : storage.GetEntries())
			{
				auto* const logic = static_cast<TestPlayerLogic*>(entry.instance);
				NOX_ASSERT(logic->GetEntity().raw == entity.raw, u"EntityLogicが保持するentityが不正です");
				method.invoke(
					entry.instance,
					world,
					*world.TryGetArchetype(entry.entity),
					world.GetArchetypeLocation(entry.entity),
					entry.entity);
			}
		}

		auto* const logic = static_cast<TestPlayerLogic*>(storage.GetEntries()[0].instance);
		NOX_ASSERT(logic->process0_count == 1 && logic->process1_count == 1 && logic->process2_count == 1,
			u"EntityLogicの各メソッドが1回ずつ呼ばれていません");
		//	nox::EntityCommands&を引数に並べたメソッドへ、Worldへのビューが束縛されている。
		NOX_ASSERT(logic->last_alive, u"nox::EntityCommandsが引数として届いていません");
		//	Process0でHealthの3、Process2でVelocityの2が足される。
		NOX_ASSERT(world.TryGetComponent<TestPosition>(entity)->x == 10.0f,
			u"EntityLogicの書き込み結果が不正です");
		NOX_ASSERT(world.TryGetComponent<TestPosition>(entity)->y == 1.0f,
			u"Serviceをポインタで受けるメソッドの書き込み結果が不正です");
		//	ポインタ経由と参照経由で同じServiceインスタンスに届いている。
		NOX_ASSERT(counter_service->call_count == 2, u"Serviceが引数として届いていません");

		storage.DestroyInstance(entity);
		NOX_ASSERT(storage.Contains(entity) == false, u"EntityLogicのインスタンスが破棄されていません");

		world.DestroyEntity(entity);
	}

	/// @brief UpdaterGraphのレイヤリングを、Worldを介さず宣言だけで検証する。
	/// @details 宣言はスタック上に手で組み立てる。ヒープも確保しないので、実行経路と同じ条件で走る。
	void TestUpdaterGraphLayering()
	{
		//	Serviceは型情報のアドレスで同一性を見る。ここでは1種類だけ使う。
		static constexpr nox::ServiceAccess k_service_write[]{
			nox::ServiceAccess{ .type = &nox::reflection::Typeof<TestCounterService>(), .write = true },
		};
		static constexpr nox::ServiceAccess k_service_read[]{
			nox::ServiceAccess{ .type = &nox::reflection::Typeof<TestCounterService>(), .write = false },
		};

		constexpr nox::uint32 k_logic_group = 0u;

		//	登録順に並べる。衝突辺は必ず登録順の小さい方から大きい方へ張られる。
		const std::array<nox::UpdaterNodeAccess, 8> accesses{
			//	n0: S1 Positionを書く。先行ノードがないのでlayer 0。
			nox::UpdaterNodeAccess{
				.read_write_mask = nox::MakeComponentMask<TestPosition>(),
				.write_mask = nox::MakeComponentMask<TestPosition>(),
			},
			//	n1: S2 Positionを読む。S1と衝突してlayer 1。
			nox::UpdaterNodeAccess{
				.read_write_mask = nox::MakeComponentMask<TestPosition>(),
			},
			//	n2: S3 Healthを書く。誰とも衝突しないのでlayer 0。
			nox::UpdaterNodeAccess{
				.read_write_mask = nox::MakeComponentMask<TestHealth>(),
				.write_mask = nox::MakeComponentMask<TestHealth>(),
			},
			//	n3: S4 PositionとHealthを読む。S1(layer0)とS3(layer0)に衝突。S2とは読み同士なので衝突しない → layer 1。
			nox::UpdaterNodeAccess{
				.read_write_mask = nox::MakeComponentMask<TestPosition, TestHealth>(),
			},
			//	n4: L1.Process0 Positionを書く。S2(layer1)・S4(layer1)の読みと衝突 → layer 2。
			nox::UpdaterNodeAccess{
				.read_write_mask = nox::MakeComponentMask<TestPosition>(),
				.write_mask = nox::MakeComponentMask<TestPosition>(),
				.group_index = k_logic_group,
			},
			//	n5: L1.Process1 Healthを読む。宣言はProcess0と重ならないが、同一EntityLogic型なので必ず後続 → layer 3。
			nox::UpdaterNodeAccess{
				.read_write_mask = nox::MakeComponentMask<TestHealth>(),
				.group_index = k_logic_group,
			},
			//	n6: S5 Serviceを書く。ComponentDataに触れないのでlayer 0。
			nox::UpdaterNodeAccess{
				.service_accesses = std::span<const nox::ServiceAccess>(k_service_write),
			},
			//	n7: S6 Serviceを読む。S5と衝突 → layer 1。
			nox::UpdaterNodeAccess{
				.service_accesses = std::span<const nox::ServiceAccess>(k_service_read),
			},
		};

		std::array<nox::uint32, 8> layer_indices{};
		const nox::uint32 layer_count = nox::BuildUpdaterLayerIndices(
			std::span<const nox::UpdaterNodeAccess>(accesses),
			std::span<nox::uint32>(layer_indices));

		NOX_ASSERT(layer_indices[0] == 0u, u"書き込みだけのノードは先頭レイヤーに置かれるはずです");
		NOX_ASSERT(layer_indices[1] == 1u, u"書き込みを読むノードが直列化されていません");
		NOX_ASSERT(layer_indices[2] == 0u, u"衝突しないノードが不要に直列化されています");
		NOX_ASSERT(layer_indices[3] == 1u, u"読み同士が衝突扱いになっています");
		NOX_ASSERT(layer_indices[4] == 2u, u"読み手の後ろに書き手が置かれていません");
		NOX_ASSERT(layer_indices[5] == 3u, u"同一EntityLogic型のメソッドが直列化されていません");
		NOX_ASSERT(layer_indices[6] == 0u, u"Serviceだけを触るノードの配置が不正です");
		NOX_ASSERT(layer_indices[7] == 1u, u"同一Serviceへの書き込みと読み取りが直列化されていません");
		NOX_ASSERT(layer_count == 4u, u"レイヤー数が不正です");

		//	衝突判定そのものの規則。読み同士は並列、書きが絡めば直列。
		NOX_ASSERT(nox::ConflictsUpdaterNodeAccess(accesses[1], accesses[3]) == false,
			u"読み同士が衝突しています");
		NOX_ASSERT(nox::ConflictsUpdaterNodeAccess(accesses[0], accesses[1]),
			u"同一ComponentDataのRWが衝突していません");
		NOX_ASSERT(nox::ConflictsUpdaterNodeAccess(accesses[4], accesses[5]),
			u"同一EntityLogic型のメソッドが衝突していません");
		NOX_ASSERT(nox::ConflictsUpdaterNodeAccess(accesses[6], accesses[7]),
			u"同一Serviceのwrite/readが衝突していません");
	}

	/// @brief 引数リストからServiceのアクセス宣言が導出される。
	void TestServiceAccessDeclaration()
	{
		using WriteSignature = nox::EntitySignature<TestPosition&, TestCounterService*>;
		using ReadSignature = nox::EntitySignature<const TestCounterService&>;

		const std::span<const nox::ServiceAccess> write_accesses = WriteSignature::GetServiceAccesses();
		NOX_ASSERT(write_accesses.size() == 1u, u"Serviceのアクセス宣言が導出されていません");
		NOX_ASSERT(write_accesses[0].type == &nox::reflection::Typeof<TestCounterService>(),
			u"Serviceの型情報が一致しません");
		NOX_ASSERT(write_accesses[0].write, u"非constのServiceが書き込み扱いになっていません");

		const std::span<const nox::ServiceAccess> read_accesses = ReadSignature::GetServiceAccesses();
		NOX_ASSERT(read_accesses.size() == 1u, u"const参照のServiceが宣言に載っていません");
		NOX_ASSERT(read_accesses[0].write == false, u"const参照のServiceが書き込み扱いになっています");

		//	ComponentDataとEntityCommandsはServiceの宣言に算入されない。
		NOX_ASSERT((nox::EntitySignature<TestPosition&, nox::EntityCommands&>::GetServiceAccesses().empty()),
			u"Service以外の引数がServiceの宣言に混ざっています");

		//	記述子経由でも同じ宣言が読める。
		const nox::EntitySystemTypeDescriptor* const move_system = FindEntitySystemType("TestMoveSystem");
		NOX_ASSERT(move_system != nullptr && move_system->get_service_accesses != nullptr,
			u"EntitySystemの記述子にServiceの宣言が載っていません");
		NOX_ASSERT(move_system != nullptr && move_system->get_service_accesses().empty(),
			u"Serviceを宣言していないSystemにServiceの宣言が載っています");
	}

	/// @brief 生成器が名前を書けない型でも、手書きの特殊化で同じ経路に載る。
	/// @details Worldの表は経由せず、記述子を直接組み立てて呼び出す。
	void TestManualEntityLogicMethodTable(nox::World& world)
	{
		using ManualLogic = nox::test::ecs::manual::ManualHealthLogic;

		static constexpr nox::EntityLogicTypeDescriptor k_descriptor =
			nox::MakeEntityLogicTypeDescriptor<ManualLogic>();

		NOX_ASSERT(k_descriptor.get_methods().size() == 1u, u"手書きのメソッド表が読めていません");
		NOX_ASSERT(k_descriptor.make_required_mask() == nox::MakeComponentMask<TestHealth>(),
			u"手書きのメソッド表から必須ComponentDataが導出されていません");
		//	Worldの表には載らない(生成器から見えない型のため)。
		NOX_ASSERT(FindEntityLogicType("ManualHealthLogic") == nullptr,
			u"生成コードから見えないはずの型が表に載っています");

		nox::EntityLogicStorage storage(k_descriptor);
		const nox::EntityId entity = world.CreateEntity();
		world.AddComponent<TestHealth>(entity)->value = 7;

		storage.CreateInstance(world, entity);
		NOX_ASSERT(storage.GetEntries().size() == 1u, u"手書き経路でインスタンスが生成されていません");

		const nox::EntityLogicStorage::Entry& entry = storage.GetEntries()[0];
		k_descriptor.get_methods()[0].invoke(
			entry.instance,
			world,
			*world.TryGetArchetype(entry.entity),
			world.GetArchetypeLocation(entry.entity),
			entry.entity);

		NOX_ASSERT(static_cast<ManualLogic*>(entry.instance)->tick_count == 1,
			u"手書き経路のメソッドが呼ばれていません");
		NOX_ASSERT(world.TryGetComponent<TestHealth>(entity)->value == 6,
			u"手書き経路の書き込み結果が不正です");

		storage.DestroyInstance(entity);
		world.DestroyEntity(entity);
	}
}

void nox::test::TestEntityEcs()
{
	TestComponentTypeRegistry();
	TestArchetypeStorage();

	TestServiceAccessDeclaration();
	TestUpdaterGraphLayering();

	nox::World world;
	TestWorldStructuralChange(world);
	TestEntitySystemExecution(world);
	TestParallelForEachEntitySystem(world);
	TestEntityLogicLifecycle(world);
	TestManualEntityLogicMethodTable(world);
}
