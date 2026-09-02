// Copyright (C) 2026 NOX ENGINE All rights reserved.

/// @file	entity_ecs_test.h
/// @brief	ECSセルフテスト用の型。
/// @details ここに定義した型は、マクロも登録呼び出しも書かずにWorldへ購読される。
///          リフレクション生成コードが nox::GetEntitySystemTypes() / nox::GetEntityLogicTypes()
///          の表を書き出すため、「ヘッダに定義するだけ」が成立していることの実証を兼ねる。
#pragma once
#include	"../component_type.h"
#include	"../service.h"
#include	"../entity_system.h"
#include	"../entity_logic.h"
#include	"../entity_logic_attribute.h"

namespace nox::test::ecs
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

	/// @brief 定義しただけでWorldに購読されるSystem。関数名はOnUpdateで固定。
	class TestMoveSystem final : public nox::EntitySystem<nox::test::ecs::TestMoveSystem>
	{
	public:
		void OnUpdate(nox::EntityId entity, nox::test::ecs::TestPosition& position, const nox::test::ecs::TestVelocity& velocity)
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
	class TestPlayerLogic final : public nox::EntityLogic<nox::test::ecs::TestPlayerLogic>
	{
	public:
		inline TestPlayerLogic(nox::World& world, const nox::EntityId entity)noexcept :
			nox::EntityLogic<nox::test::ecs::TestPlayerLogic>(world, entity)
		{
		}

		NOX_ATTR(nox::attr::EntityLogicMethod(nox::SystemPhaseType::Update))
		void Process0(nox::test::ecs::TestPosition& position, const nox::test::ecs::TestHealth& health)
		{
			//	個別の状態は普通のメンバとして持てる(ComponentData化しなくてよい)。
			++process0_count;
			position.x += static_cast<nox::float32>(health.value);
		}

		NOX_ATTR(nox::attr::EntityLogicMethod(nox::SystemPhaseType::Update))
		void Process1(nox::test::ecs::TestPosition& position, nox::test::ecs::TestCounterService* service)
		{
			//	Serviceをポインタで受けた場合、未登録ならnullptrが渡る。
			++process1_count;
			position.y += 1.0f;
			if (service != nullptr)
			{
				++service->call_count;
			}
		}

		NOX_ATTR(nox::attr::EntityLogicMethod(nox::SystemPhaseType::Update))
		void Process2(
			nox::test::ecs::TestPosition& position,
			const nox::test::ecs::TestVelocity& velocity,
			nox::test::ecs::TestCounterService& service)
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
	};
}
