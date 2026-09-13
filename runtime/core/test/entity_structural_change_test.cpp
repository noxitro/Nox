//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	entity_structural_change_test.cpp
///	@brief	構造変更2系統(即時系 / 遅延系)の境界と、即時系にかかる制約の検証。
///	@details	検証しているのは次の3点。
///
///				1. 即時系の制約
///				   nox::World::GetStructuralChangePermission() が、列挙中に
///				   nox::StructuralChangePermission::DeniedDuringIteration を返すこと。
///				   これは即時系4本(CreateEntity / DestroyEntity / AddComponent / RemoveComponent)が
///				   実際に分岐している唯一の判定点なので、ここを見れば
///				   「列挙中に即時系を呼んだら弾かれる」ことが確認できる。
///
///				2. 遅延系がフェーズ終端まで反映されないこと
///				   列挙中に積んだ Destroy / Add / Remove が、列挙直後にはまだ効いておらず、
///				   Playbackポイント(nox::World::FlushEntityCommands)を通して初めて反映されること。
///
///				3. コマンドバッファの容量超過
///				   コマンド枠とペイロード枠のどちらが尽きても、記録が false を返すだけで
///				   確保にも未定義動作にも走らないこと。
///
///	@note		即時系を「実際に呼んで」弾かれることは、ここでは検証していない。
///				弾く経路には NOX_ASSERT が入っており、Debug では ::_wassert で
///				プロセスごと停止するため gtest から踏めないからである。
///				代わりに、その NOX_ASSERT と早期returnの両方が参照している
///				判定値そのものを、実際の列挙の内側で観測している。

#include	"pch.h"

//	core_test の pch.h は gtest しか載せていない (test_new_delete.cpp の都合)。
//	core のヘッダは kernel / reflection の基盤型に依存するので、main.cpp と同じ順で先に入れる。
#include	"../../kernel/kernel.h"
#include	"../../reflection/reflection.h"

#include	"../world.h"
#include	"../entity_query.h"
#include	"../entity_commands.h"
#include	"../entity_command_buffer.h"

namespace
{
	struct StructuralPosition : nox::IComponentData
	{
		nox::float32 x;
		nox::float32 y;
	};

	struct StructuralHealth : nox::IComponentData
	{
		nox::int32 value;
	};

	/// @brief 列挙の内側から観測を行うだけの購読者。
	/// @details Worldは保持しない。フェーズ中に許される操作は nox::EntityCommands& で受け取る、
	///          という本来の書き方をそのままなぞっている。
	class PermissionProbe final
	{
	public:
		void Visit(
			const nox::EntityId entity,
			StructuralPosition& position,
			nox::EntityCommands& commands)
		{
			++visit_count;
			last_permission = world->GetStructuralChangePermission();
			position.x += 1.0f;
			last_alive = commands.IsAlive(entity);
		}

		nox::World* world = nullptr;
		nox::int32 visit_count = 0;
		bool last_alive = false;
		nox::StructuralChangePermission last_permission = nox::StructuralChangePermission::Allowed;
	};

	/// @brief 列挙中に遅延系だけを使って構造を変える購読者。
	class DeferredSpawnProbe final
	{
	public:
		void Visit(
			const nox::EntityId entity,
			const StructuralPosition& position,
			nox::EntityCommands& commands)
		{
			(void)position;
			//	生成はその場でIdが返る(EntityRecordを1件触るだけでArchetypeを動かさないため)。
			const nox::EntityId spawned = commands.Create();
			spawned_entities[static_cast<size_t>(spawn_count)] = spawned;
			//	初期値つきでComponentDataの追加を予約する。反映はPlaybackポイント。
			commands.Add<StructuralHealth>(spawned, StructuralHealth{ .value = 40 + spawn_count });
			++spawn_count;

			//	自分自身の破棄も予約できる。列挙中に実体は消えない。
			commands.Destroy(entity);
		}

		std::array<nox::EntityId, 8> spawned_entities{};
		nox::int32 spawn_count = 0;
	};

	/// @brief StructuralPositionを持つentityを列挙するQueryを作る。
	[[nodiscard]] nox::ComponentMask MakePositionMask()
	{
		return nox::MakeComponentMask<StructuralPosition>();
	}
}

//	=====================================================================================
//	1. 即時系の制約
//	=====================================================================================

///	@brief	ネガティブコントロール: 列挙していないときは即時系が許可され、実際に効く。
TEST(EntityStructuralChange, ImmediateIsAllowedOutsideIteration)
{
	nox::World world;

	EXPECT_EQ(world.GetStructuralChangePermission(), nox::StructuralChangePermission::Allowed);
	EXPECT_FALSE(world.IsIteratingEntities());
	EXPECT_FALSE(world.IsExecutingSystemPhase());

	const nox::EntityId entity = world.CreateEntity();
	EXPECT_TRUE(world.IsAlive(entity));

	//	即時系はその場で反映される。生成→初期化→参照が一続きに書ける。
	StructuralPosition* const position = world.AddComponent<StructuralPosition>(entity);
	ASSERT_NE(position, nullptr);
	position->x = 3.0f;

	EXPECT_TRUE(world.HasComponent<StructuralPosition>(entity));
	ASSERT_NE(world.TryGetComponent<StructuralPosition>(entity), nullptr);
	EXPECT_FLOAT_EQ(world.TryGetComponent<StructuralPosition>(entity)->x, 3.0f);

	world.RemoveComponent<StructuralPosition>(entity);
	EXPECT_FALSE(world.HasComponent<StructuralPosition>(entity));

	world.DestroyEntity(entity);
	EXPECT_FALSE(world.IsAlive(entity));
	EXPECT_EQ(world.GetStructuralChangePermission(), nox::StructuralChangePermission::Allowed);
}

///	@brief	ポジティブコントロール: 列挙中は即時系が弾かれる状態になる。
///	@details	即時系4本が分岐している判定値そのものを、実際の列挙の内側で観測する。
TEST(EntityStructuralChange, ImmediateIsDeniedDuringIteration)
{
	nox::World world;

	for (nox::uint32 index = 0u; index < 3u; ++index)
	{
		const nox::EntityId entity = world.CreateEntity();
		world.AddComponent<StructuralPosition>(entity)->x = static_cast<nox::float32>(index);
	}

	nox::EntityQuery query;
	world.BuildQuery(query, MakePositionMask());

	PermissionProbe probe;
	probe.world = &world;

	//	列挙に入る前は許可されている。
	ASSERT_EQ(world.GetStructuralChangePermission(), nox::StructuralChangePermission::Allowed);

	using Invoker = nox::detail::EntityInvoker<nox::EntityId, StructuralPosition&, nox::EntityCommands&>;
	Invoker::ForEachEntity(world, query, probe, &PermissionProbe::Visit);

	EXPECT_EQ(probe.visit_count, 3);
	EXPECT_TRUE(probe.last_alive);

	//	これがこのテストの本体。列挙の内側では即時系が弾かれる状態になっている。
	EXPECT_EQ(probe.last_permission, nox::StructuralChangePermission::DeniedDuringIteration);

	//	列挙を抜けたら元に戻る(スコープが釣り合っている)。
	EXPECT_EQ(world.GetStructuralChangePermission(), nox::StructuralChangePermission::Allowed);
	EXPECT_FALSE(world.IsIteratingEntities());
}

///	@brief	列挙スコープは入れ子にできる(Chunk単位の列挙が外側の列挙の内側に入る形)。
TEST(EntityStructuralChange, IterationScopeNests)
{
	nox::World world;
	const nox::EntityId entity = world.CreateEntity();
	world.AddComponent<StructuralPosition>(entity);

	world.EnterEntityIteration();
	EXPECT_EQ(world.GetStructuralChangePermission(), nox::StructuralChangePermission::DeniedDuringIteration);
	world.EnterEntityIteration();
	EXPECT_EQ(world.GetStructuralChangePermission(), nox::StructuralChangePermission::DeniedDuringIteration);
	world.LeaveEntityIteration();

	//	内側を抜けただけでは解除されない。
	EXPECT_EQ(world.GetStructuralChangePermission(), nox::StructuralChangePermission::DeniedDuringIteration);
	world.LeaveEntityIteration();
	EXPECT_EQ(world.GetStructuralChangePermission(), nox::StructuralChangePermission::Allowed);
}

///	@brief	列挙スコープは別スレッドから見ても立っている(並列列挙中の即時系を弾くため)。
///	@details	状態は単一のatomicワードなので、配る側のスレッドが立てたブロックを
///				ワーカー側から観測できる。並列化したときに「ワーカーから即時系を呼んだ」を
///				弾けることの土台になる。
TEST(EntityStructuralChange, IterationScopeIsVisibleFromOtherThreads)
{
	nox::World world;

	world.EnterEntityIteration();

	nox::StructuralChangePermission observed = nox::StructuralChangePermission::Allowed;
	std::thread worker([&world, &observed]()
		{
			observed = world.GetStructuralChangePermission();
		});
	worker.join();

	EXPECT_EQ(observed, nox::StructuralChangePermission::DeniedDuringIteration);

	world.LeaveEntityIteration();
	EXPECT_EQ(world.GetStructuralChangePermission(), nox::StructuralChangePermission::Allowed);
}

//	=====================================================================================
//	2. 遅延系
//	=====================================================================================

///	@brief	列挙中に積んだ構造変更は、列挙中は反映されず、Playbackポイントで反映される。
TEST(EntityStructuralChange, DeferredCommandsApplyOnlyAtPlayback)
{
	nox::World world;

	std::array<nox::EntityId, 3> sources{};
	for (nox::uint32 index = 0u; index < sources.size(); ++index)
	{
		sources[index] = world.CreateEntity();
		world.AddComponent<StructuralPosition>(sources[index])->x = static_cast<nox::float32>(index);
	}

	nox::EntityQuery query;
	world.BuildQuery(query, MakePositionMask());

	DeferredSpawnProbe probe;
	using Invoker = nox::detail::EntityInvoker<nox::EntityId, const StructuralPosition&, nox::EntityCommands&>;
	Invoker::ForEachEntity(world, query, probe, &DeferredSpawnProbe::Visit);

	ASSERT_EQ(probe.spawn_count, 3);

	//	--- 列挙直後 (Playback前) ---
	for (const nox::EntityId source : sources)
	{
		//	Destroyを積んだだけなので、まだ生きている。
		EXPECT_TRUE(world.IsAlive(source));
	}
	for (nox::int32 index = 0; index < probe.spawn_count; ++index)
	{
		const nox::EntityId spawned = probe.spawned_entities[static_cast<size_t>(index)];
		//	Createは即時にIdを返すので、生成そのものは既に効いている。
		EXPECT_TRUE(world.IsAlive(spawned));
		//	ただしComponentDataの追加は遅延なので、まだ持っていない。
		EXPECT_FALSE(world.HasComponent<StructuralHealth>(spawned));
	}

	//	--- Playbackポイント (フェーズ終端に相当) ---
	world.FlushEntityCommands();

	for (const nox::EntityId source : sources)
	{
		EXPECT_FALSE(world.IsAlive(source));
	}
	for (nox::int32 index = 0; index < probe.spawn_count; ++index)
	{
		const nox::EntityId spawned = probe.spawned_entities[static_cast<size_t>(index)];
		EXPECT_TRUE(world.IsAlive(spawned));
		ASSERT_TRUE(world.HasComponent<StructuralHealth>(spawned));
		const StructuralHealth* const health = world.TryGetComponent<StructuralHealth>(spawned);
		ASSERT_NE(health, nullptr);
		//	Addで渡した初期値がコマンドバッファ経由で運ばれている。
		EXPECT_EQ(health->value, 40 + index);
	}
}

///	@brief	遅延系のRemoveもPlaybackポイントで反映される。
TEST(EntityStructuralChange, DeferredRemoveComponentAppliesAtPlayback)
{
	nox::World world;

	const nox::EntityId entity = world.CreateEntity();
	world.AddComponent<StructuralPosition>(entity);
	world.AddComponent<StructuralHealth>(entity)->value = 7;

	world.EnterEntityIteration();
	{
		nox::EntityCommands commands(world);
		commands.Remove<StructuralHealth>(entity);
		//	列挙中は効かない。
		EXPECT_TRUE(world.HasComponent<StructuralHealth>(entity));
	}
	world.LeaveEntityIteration();

	world.FlushEntityCommands();
	EXPECT_FALSE(world.HasComponent<StructuralHealth>(entity));
	EXPECT_TRUE(world.HasComponent<StructuralPosition>(entity));
}

///	@brief	同一フェーズ内で Destroy されたentityへの Add は、Playbackで黙って捨てられる。
///	@details	ごく普通に起きる組み合わせ。ダメージ処理のSystemが破棄を積み、別のSystem/EntityLogicが
///				同じentityへ状態を付与する、という順で積まれると、Playbackは記録順に
///				Destroy → Add と再生するのでgenerationが一致しなくなる。
///
///				これは即時系の誤用(stale handleを渡した)ではなく、遅延系では正常に起こりうる競合なので、
///				アサートで止めてはならない。Removeが昔から黙ってreturnしているのと同じ扱いにする。
TEST(EntityStructuralChange, DeferredAddOnEntityDestroyedInSamePhaseIsSkipped)
{
	nox::World world;

	const nox::EntityId victim = world.CreateEntity();
	world.AddComponent<StructuralPosition>(victim);

	//	巻き添えにならないことを見るための隣人。
	const nox::EntityId bystander = world.CreateEntity();
	world.AddComponent<StructuralPosition>(bystander);

	world.EnterEntityIteration();
	{
		nox::EntityCommands commands(world);
		//	System A: 破棄を積む。
		commands.Destroy(victim);
		//	System B: 同じフェーズで、同じentityへ状態を付与しようとする。
		commands.Add<StructuralHealth>(victim, StructuralHealth{ .value = 99 });
		//	後続のコマンドが打ち切られないことも確認する。
		commands.Add<StructuralHealth>(bystander, StructuralHealth{ .value = 7 });
	}
	world.LeaveEntityIteration();

	//	修正前はここで NOX_ASSERT が発火し、Debugでは ::_wassert でプロセスごと停止する。
	world.FlushEntityCommands();

	EXPECT_FALSE(world.IsAlive(victim));
	EXPECT_FALSE(world.HasComponent<StructuralHealth>(victim));

	//	破棄済みへのAddが捨てられても、同じPlaybackの後続コマンドは適用される。
	EXPECT_TRUE(world.IsAlive(bystander));
	ASSERT_TRUE(world.HasComponent<StructuralHealth>(bystander));
	EXPECT_EQ(world.TryGetComponent<StructuralHealth>(bystander)->value, 7);
}

///	@brief	初期値を渡さないAddは、ゼロ初期化のまま追加される。
TEST(EntityStructuralChange, DeferredAddWithoutValueIsZeroInitialized)
{
	nox::World world;

	world.EnterEntityIteration();
	nox::EntityId spawned{ 0u };
	{
		nox::EntityCommands commands(world);
		spawned = commands.Create();
		commands.Add<StructuralHealth>(spawned);
	}
	world.LeaveEntityIteration();

	world.FlushEntityCommands();
	ASSERT_TRUE(world.HasComponent<StructuralHealth>(spawned));
	const StructuralHealth* const health = world.TryGetComponent<StructuralHealth>(spawned);
	ASSERT_NE(health, nullptr);
	EXPECT_EQ(health->value, 0);
}

//	=====================================================================================
//	3. コマンドバッファの容量超過
//	=====================================================================================

///	@brief	コマンド枠が尽きたら記録がfalseを返す。
TEST(EntityCommandBufferCapacity, RejectsWhenCommandSlotsAreExhausted)
{
	nox::EntityCommandBuffer<2u, 256u> buffer;
	const nox::EntityId entity{ .generation = 1u, .index = 1u };

	EXPECT_TRUE(buffer.TryDestroy(entity));
	EXPECT_TRUE(buffer.TryDestroy(entity));
	//	3件目は枠が無い。確保もせず、falseを返すだけ。
	EXPECT_FALSE(buffer.TryDestroy(entity));
	EXPECT_EQ(buffer.GetLength(), 2u);

	buffer.Clear();
	EXPECT_EQ(buffer.GetLength(), 0u);
	EXPECT_EQ(buffer.GetPayloadLength(), 0u);
	//	Clearの後はまた積める。
	EXPECT_TRUE(buffer.TryDestroy(entity));
}

///	@brief	初期値を運ぶペイロード枠が尽きたら、コマンド枠が余っていても記録がfalseを返す。
TEST(EntityCommandBufferCapacity, RejectsWhenPayloadBytesAreExhausted)
{
	//	ペイロードは StructuralHealth 1個分しか無い。
	nox::EntityCommandBuffer<8u, sizeof(StructuralHealth)> buffer;
	const nox::EntityId entity{ .generation = 1u, .index = 1u };
	const StructuralHealth value{ .value = 5 };
	const nox::ComponentTypeInfo& type_info = nox::ComponentTypeOf<StructuralHealth>();

	EXPECT_TRUE(buffer.TryAddComponent(entity, type_info, &value));
	EXPECT_EQ(buffer.GetPayloadLength(), sizeof(StructuralHealth));

	//	コマンド枠(8)はまだ余っているが、ペイロードが尽きている。
	EXPECT_FALSE(buffer.TryAddComponent(entity, type_info, &value));
	EXPECT_EQ(buffer.GetLength(), 1u);

	//	初期値を運ばないAddとRemoveはペイロードを使わないので、まだ積める。
	EXPECT_TRUE(buffer.TryAddComponent(entity, type_info, nullptr));
	EXPECT_TRUE(buffer.TryRemoveComponent(entity, type_info));
	EXPECT_EQ(buffer.GetLength(), 3u);
}

///	@brief	記録した初期値がそのまま読み戻せる。
TEST(EntityCommandBufferCapacity, CarriesComponentPayload)
{
	nox::EntityCommandBuffer<4u, 256u> buffer;
	const nox::EntityId entity{ .generation = 2u, .index = 9u };
	const StructuralHealth value{ .value = 1234 };

	ASSERT_TRUE(buffer.TryAddComponent(entity, nox::ComponentTypeOf<StructuralHealth>(), &value));

	nox::EntityCommand command{};
	ASSERT_TRUE(buffer.TryGet(0u, command));
	EXPECT_EQ(command.type, nox::EntityCommandType::AddComponent);
	EXPECT_EQ(command.entity_raw, entity.raw);
	EXPECT_EQ(command.payload_size, sizeof(StructuralHealth));

	const void* const payload = buffer.TryGetPayload(command);
	ASSERT_NE(payload, nullptr);
	EXPECT_EQ(static_cast<const StructuralHealth*>(payload)->value, 1234);
}

///	@brief	high-water markはClearを跨いで最大値を保持する。
///	@details	溢れたら abort する設計なので、出荷前に容量を実測で根拠づけるための安全弁。
TEST(EntityCommandBufferCapacity, TracksHighWaterMarkAcrossClear)
{
	nox::EntityCommandBuffer<8u, 256u> buffer;
	const nox::EntityId entity{ .generation = 1u, .index = 1u };
	const StructuralHealth value{ .value = 3 };
	const nox::ComponentTypeInfo& type_info = nox::ComponentTypeOf<StructuralHealth>();

	EXPECT_EQ(buffer.GetPeakLength(), 0u);

	//	3件積んで流す。
	ASSERT_TRUE(buffer.TryDestroy(entity));
	ASSERT_TRUE(buffer.TryAddComponent(entity, type_info, &value));
	ASSERT_TRUE(buffer.TryRemoveComponent(entity, type_info));
	buffer.Clear();

	EXPECT_EQ(buffer.GetPeakLength(), 3u);
	EXPECT_EQ(buffer.GetPeakPayloadLength(), sizeof(StructuralHealth));
	//	Clear自体は現在値だけを戻す。
	EXPECT_EQ(buffer.GetLength(), 0u);

	//	次のフェーズが少なければ、最大値は下がらない。
	ASSERT_TRUE(buffer.TryDestroy(entity));
	buffer.Clear();
	EXPECT_EQ(buffer.GetPeakLength(), 3u);

	//	増えたときだけ更新される。
	for (nox::uint32 index = 0u; index < 5u; ++index)
	{
		ASSERT_TRUE(buffer.TryDestroy(entity));
	}
	buffer.Clear();
	EXPECT_EQ(buffer.GetPeakLength(), 5u);
}

///	@brief	Worldからもhigh-water markを引ける(容量を出荷前に裏付けるため)。
TEST(EntityCommandBufferCapacity, WorldExposesHighWaterMark)
{
	nox::World world;
	EXPECT_EQ(world.GetEntityCommandPeakLength(), 0u);

	const nox::EntityId entity = world.CreateEntity();
	world.AddComponent<StructuralPosition>(entity);

	world.EnterEntityIteration();
	{
		nox::EntityCommands commands(world);
		commands.Add<StructuralHealth>(entity, StructuralHealth{ .value = 1 });
		commands.Destroy(entity);
	}
	world.LeaveEntityIteration();
	world.FlushEntityCommands();

	EXPECT_EQ(world.GetEntityCommandPeakLength(), 2u);
	EXPECT_EQ(world.GetEntityCommandPeakPayloadLength(), sizeof(StructuralHealth));
	EXPECT_LT(world.GetEntityCommandPeakLength(), nox::World::GetEntityCommandCapacity());
}

///	@brief	Playbackを開始したら新しい記録を受け付けない。
TEST(EntityCommandBufferCapacity, RejectsRecordingDuringPlayback)
{
	nox::EntityCommandBuffer<4u, 256u> buffer;
	const nox::EntityId entity{ .generation = 1u, .index = 1u };

	ASSERT_TRUE(buffer.TryDestroy(entity));
	buffer.BeginPlayback();

	EXPECT_FALSE(buffer.TryDestroy(entity));
	EXPECT_FALSE(buffer.TryRemoveComponent(entity, nox::ComponentTypeOf<StructuralHealth>()));
	EXPECT_EQ(buffer.GetLength(), 1u);
}
