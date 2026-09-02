// Copyright (C) 2026 NOX ENGINE All rights reserved.

/// @file	entity_ecs_test.cpp
/// @brief	Archetypeストレージ / シグネチャ解析 / EntitySystem / EntityLogic のテスト。
#include "pch.h"
#include "test.h"

#include "../world.h"
#include "../entity_system.h"
#include "../entity_logic.h"
#include "../../kernel/assertion.h"

namespace
{
	struct TestPosition : nox::IComponentData
	{
		nox::float32 x;
		nox::float32 y;
	};

	struct TestVelocity : nox::IComponentData
	{
		nox::float32 x;
		nox::float32 y;
	};

	struct TestHealth : nox::IComponentData
	{
		nox::int32 value;
	};

	class TestCounterService final : public nox::Service
	{
		NOX_DECLARE_OBJECT(TestCounterService, nox::Service);
	public:
		nox::int32 call_count = 0;
	};

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

	//	引数リストがそのままメソッドの宣言として解釈される。
	struct SignatureProbe
	{
		void Process00(nox::EntityId entity, TestPosition& position, TestCounterService* service);
	};
	static_assert(nox::EntityMethod<decltype(&SignatureProbe::Process00)>);
	static_assert(std::same_as<
		nox::EntityMethodTraits<decltype(&SignatureProbe::Process00)>::Signature,
		nox::EntitySignature<nox::EntityId, TestPosition&, TestCounterService*>>);

#pragma endregion

#pragma region テスト用EntitySystem / EntityLogic

	/// @brief 定義しただけでWorldに購読されるSystem。関数名はOnUpdateで固定。
	class TestMoveSystem final : public nox::EntitySystem<TestMoveSystem>
	{
		NOX_DECLARE_ENTITY_SYSTEM(TestMoveSystem);
	public:
		void OnUpdate(nox::EntityId entity, TestPosition& position, const TestVelocity& velocity)
		{
			position.x += velocity.x;
			position.y += velocity.y;
			last_entity = entity;
			++processed_count;
		}

		nox::int32 processed_count = 0;
		nox::EntityId last_entity{ 0u };
	};

	/// @brief 必須ComponentDataは基底のテンプレート引数ではなく、メソッド群の引数から導出される。
	/// @details メソッド名は任意、引数リストも任意。ここでは
	///          「ComponentDataのみ」「Serviceをポインタで」「Serviceを参照で」の3形を並べている。
	class TestPlayerLogic final : public nox::EntityLogic<TestPlayerLogic>
	{
	public:
		inline TestPlayerLogic(nox::World& world, const nox::EntityId entity)noexcept :
			nox::EntityLogic<TestPlayerLogic>(world, entity)
		{
		}

		void Process0(TestPosition& position, const TestHealth& health)
		{
			//	個別の状態は普通のメンバとして持てる(ComponentData化しなくてよい)。
			++process0_count;
			position.x += static_cast<nox::float32>(health.value);
		}

		void Process1(TestPosition& position, TestCounterService* service)
		{
			//	Serviceをポインタで受けた場合、未登録ならnullptrが渡る。
			++process1_count;
			position.y += 1.0f;
			if (service != nullptr)
			{
				++service->call_count;
			}
		}

		void Process2(TestPosition& position, const TestVelocity& velocity, TestCounterService& service)
		{
			//	Serviceを参照で受けた場合、未登録なら呼び出しごと打ち切られる(nullは渡らない)。
			//	TestVelocityはProcess2だけが宣言しているが、必須ComponentDataに算入される。
			++process2_count;
			position.x += velocity.x;
			++service.call_count;
		}

		nox::int32 process0_count = 0;
		nox::int32 process1_count = 0;
		nox::int32 process2_count = 0;

		NOX_DECLARE_ENTITY_LOGIC(TestPlayerLogic,
			NOX_ENTITY_LOGIC_METHOD(Update, &TestPlayerLogic::Process0),
			NOX_ENTITY_LOGIC_METHOD(Update, &TestPlayerLogic::Process1),
			NOX_ENTITY_LOGIC_METHOD(Update, &TestPlayerLogic::Process2));
	};

#pragma endregion

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
		//	定義しただけで型記述子が登録されている。
		bool registered = false;
		for (const nox::EntitySystemTypeDescriptor* descriptor = nox::detail::GetEntitySystemTypeListHead();
			descriptor != nullptr;
			descriptor = descriptor->next)
		{
			registered = registered || (descriptor->execute == nullptr ? false : descriptor->name.find("TestMoveSystem") != std::string_view::npos);
		}
		NOX_ASSERT(registered, u"EntitySystemが自動登録されていません");

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

	void TestEntityLogicLifecycle(nox::World& world)
	{
		const nox::EntityLogicTypeDescriptor* logic_descriptor = nullptr;
		for (const nox::EntityLogicTypeDescriptor* descriptor = nox::detail::GetEntityLogicTypeListHead();
			descriptor != nullptr;
			descriptor = descriptor->next)
		{
			if (descriptor->name.find("TestPlayerLogic") != std::string_view::npos)
			{
				logic_descriptor = descriptor;
			}
		}
		NOX_ASSERT(logic_descriptor != nullptr, u"EntityLogicが自動登録されていません");
		if (logic_descriptor == nullptr)
		{
			return;
		}

		//	必須ComponentDataは3つのメソッドの引数の和集合になる。
		//	Process2だけが宣言しているTestVelocityも含まれ、Serviceは含まれない。
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
}

void nox::test::TestEntityEcs()
{
	TestComponentTypeRegistry();
	TestArchetypeStorage();

	nox::World world;
	TestWorldStructuralChange(world);
	TestEntitySystemExecution(world);
	TestEntityLogicLifecycle(world);
}
