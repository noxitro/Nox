//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	entity_command_playback_test.cpp
///	@brief	遅延構造変更のPlaback順が、記録順に依存せず決定的であることの検証。
///	@details	遅延系のコマンドバッファはUpdaterGraphのノード1つにつき1本あり、
///				Playbackは「ノード外 → ノード番号(nox::UpdaterNode::order_index)順」で回る。
///				よって「どのワーカーがどのノードを先に走らせたか」は結果に影響しない。
///
///				ここで落とせるようにしてあるのは次の3点。
///
///				1. 決定性
///				   同じ構造変更の集合を、ノードの実行順だけ変えて記録しても、
///				   Playback後のWorldの状態が完全に一致すること。
///				   単に1回走らせて期待値と比べるのでは「たまたまその順だった」を排除できないため、
///				   4ノードの全順列(24通り)を総当たりで回し、さらに実スレッドで競争させた版も
///				   同じ結果になることを確認している。
///				   検体には順序に敏感な組み合わせ(同一entityへの Add → Remove → Add)を必ず含める。
///
///				2. ノードごとのバッファが独立していること
///				   あるノードのバッファが溢れても、別ノードのバッファの内容も残容量も変わらないこと。
///				   Worldを経由すると溢れは std::abort() なので、バッファ単体で確認する。
///
///				3. high-water markがノード単位で取れること
///				   容量の妥当性を出荷前に裏取りするための観測窓口が、分割後も残っていること。

#include	"pch.h"

//	core_test の pch.h は gtest しか載せていない (test_new_delete.cpp の都合)。
//	core のヘッダは kernel / reflection の基盤型に依存するので、main.cpp と同じ順で先に入れる。
#include	"../../kernel/kernel.h"
#include	"../../reflection/reflection.h"

#include	"../world.h"
#include	"../entity_commands.h"
#include	"../entity_command_buffer.h"
#include	"../entity_type_registry.h"
#include	"../updater_graph.h"

#include	<algorithm>
#include	<atomic>
#include	<cstdio>
#include	<thread>
#include	<vector>

namespace
{
	struct PlaybackPosition : nox::IComponentData
	{
		nox::float32 x;
		nox::float32 y;
	};

	struct PlaybackHealth : nox::IComponentData
	{
		nox::int32 value;
	};

	/// @brief 検体に使うノード数。全順列(24通り)を総当たりできる大きさにしてある。
	constexpr nox::uint32 k_node_count = 4u;
	/// @brief 検体に使うentity数。
	constexpr nox::uint32 k_entity_count = 3u;

	/// @brief Playback後のWorldの状態。これが順序によらず一致することを見る。
	struct WorldSnapshot
	{
		struct EntityState
		{
			bool alive = false;
			bool has_health = false;
			bool has_position = false;
			nox::int32 health_value = 0;

			[[nodiscard]] bool operator==(const EntityState&)const = default;
		};

		std::array<EntityState, k_entity_count> entities{};

		[[nodiscard]] bool operator==(const WorldSnapshot&)const = default;
	};

	/// @brief ノード1つが積む構造変更。
	/// @details 順序に敏感な組み合わせを意図的に入れてある。
	///          e0: node0 Add(10) → node1 Remove → node2 Add(20)  ⇒ 決定的なら value=20
	///          e1: node0 Add(1)  → node3 Remove                  ⇒ 決定的なら Healthなし
	///          e2: node1 Destroy → node3 Add(99)                 ⇒ 破棄済みへのAddは捨てられる
	void RecordNodeCommands(
		const nox::uint32 node_index,
		nox::EntityCommands& commands,
		const std::span<const nox::EntityId> entities)
	{
		switch (node_index)
		{
		case 0u:
			commands.Add<PlaybackHealth>(entities[0], PlaybackHealth{ .value = 10 });
			commands.Add<PlaybackHealth>(entities[1], PlaybackHealth{ .value = 1 });
			break;

		case 1u:
			commands.Remove<PlaybackHealth>(entities[0]);
			commands.Destroy(entities[2]);
			break;

		case 2u:
			commands.Add<PlaybackHealth>(entities[0], PlaybackHealth{ .value = 20 });
			commands.Add<PlaybackPosition>(entities[2], PlaybackPosition{ .x = 3.0f, .y = 4.0f });
			break;

		case 3u:
			commands.Remove<PlaybackHealth>(entities[1]);
			commands.Add<PlaybackHealth>(entities[2], PlaybackHealth{ .value = 99 });
			break;

		default:
			break;
		}
	}

	[[nodiscard]] WorldSnapshot CaptureSnapshot(
		nox::World& world,
		const std::span<const nox::EntityId> entities)
	{
		WorldSnapshot snapshot{};
		for (nox::uint32 index = 0u; index < k_entity_count; ++index)
		{
			WorldSnapshot::EntityState& state = snapshot.entities[index];
			state.alive = world.IsAlive(entities[index]);
			if (state.alive == false)
			{
				continue;
			}

			state.has_health = world.HasComponent<PlaybackHealth>(entities[index]);
			state.has_position = world.HasComponent<PlaybackPosition>(entities[index]);
			const PlaybackHealth* const health = world.TryGetComponent<PlaybackHealth>(entities[index]);
			state.health_value = (health != nullptr) ? health->value : 0;
		}
		return snapshot;
	}

	/// @brief 検体のentityを用意する。全てPlaybackPositionを持つ状態から始める。
	void SetupEntities(nox::World& world, const std::span<nox::EntityId> dest)
	{
		for (nox::uint32 index = 0u; index < dest.size(); ++index)
		{
			dest[index] = world.CreateEntity();
			auto* const position = world.AddComponent<PlaybackPosition>(dest[index]);
			ASSERT_NE(position, nullptr);
			*position = PlaybackPosition{ .x = static_cast<nox::float32>(index), .y = 0.0f };
		}
	}

	/// @brief ノードを指定された順に「直列で」実行して、Playback後の状態を返す。
	/// @details 記録順だけを入れ替えるための経路。スレッドを使わないので、順序は完全に制御できる。
	[[nodiscard]] WorldSnapshot RunWithNodeOrder(const std::span<const nox::uint32> node_order)
	{
		nox::World world;
		world.ReserveNodeEntityCommandBuffers(k_node_count);

		std::array<nox::EntityId, k_entity_count> entities{};
		SetupEntities(world, entities);

		//	遅延系は「フェーズ実行中または列挙中」でなければ記録できない。
		//	フェーズはWorld内部からしか開けないので、列挙スコープで代用する。
		world.EnterEntityIteration();
		for (const nox::uint32 node_index : node_order)
		{
			const nox::WorldNodeCommandScope command_scope(world, node_index);
			nox::EntityCommands commands(world);
			RecordNodeCommands(node_index, commands, entities);
		}
		world.LeaveEntityIteration();

		world.FlushEntityCommands();
		return CaptureSnapshot(world, entities);
	}

	/// @brief ノードを実スレッドで同時に走らせて、Playback後の状態を返す。
	/// @details 記録の重なりを実際に起こすための経路。全スレッドがバリアで足並みを揃えてから
	///          一斉に記録するので、コマンドバッファへの到着順はスケジューラ任せになる。
	[[nodiscard]] WorldSnapshot RunWithConcurrentNodes()
	{
		nox::World world;
		world.ReserveNodeEntityCommandBuffers(k_node_count);

		std::array<nox::EntityId, k_entity_count> entities{};
		SetupEntities(world, entities);

		world.EnterEntityIteration();
		{
			std::atomic<nox::uint32> ready_count{ 0u };
			std::atomic_bool start{ false };
			std::array<std::thread, k_node_count> threads;
			for (nox::uint32 node_index = 0u; node_index < k_node_count; ++node_index)
			{
				threads[node_index] = std::thread([&world, &entities, &ready_count, &start, node_index]()
					{
						ready_count.fetch_add(1u, std::memory_order_release);
						while (start.load(std::memory_order_acquire) == false)
						{
							std::this_thread::yield();
						}

						const nox::WorldNodeCommandScope command_scope(world, node_index);
						nox::EntityCommands commands(world);
						RecordNodeCommands(node_index, commands, entities);
					});
			}

			while (ready_count.load(std::memory_order_acquire) < k_node_count)
			{
				std::this_thread::yield();
			}
			start.store(true, std::memory_order_release);

			for (std::thread& thread : threads)
			{
				thread.join();
			}
		}
		world.LeaveEntityIteration();

		world.FlushEntityCommands();
		return CaptureSnapshot(world, entities);
	}
}

//	=====================================================================================
//	1. 決定性
//	=====================================================================================

///	@brief	ノードの実行順を全順列で入れ替えても、Playback後の状態が完全に一致すること。
///	@details	これがこの分割の本丸。記録順(=ワーカーがどのノードを先に走らせたか)は
///				24通りすべて違うのに、Playbackはノード番号順で回るため結果が動かない。
TEST(EntityCommandPlayback, IsDeterministicAcrossEveryNodeOrder)
{
	std::array<nox::uint32, k_node_count> node_order{ 0u, 1u, 2u, 3u };
	const WorldSnapshot expected = RunWithNodeOrder(node_order);

	nox::uint32 permutation_count = 0u;
	do
	{
		const WorldSnapshot actual = RunWithNodeOrder(node_order);
		++permutation_count;

		SCOPED_TRACE(::testing::Message()
			<< "node order = " << node_order[0] << node_order[1] << node_order[2] << node_order[3]);
		EXPECT_EQ(actual, expected);
	} while (std::next_permutation(node_order.begin(), node_order.end()));

	//	4! = 24通りを本当に全部回したか(ループが空振りしていないことの確認)。
	EXPECT_EQ(permutation_count, 24u);
}

///	@brief	Playback結果が、ノード番号順に再生した場合の期待値そのものであること。
///	@details	決定性(=毎回同じ)だけでは「毎回同じように間違っている」を除けない。
///				順序に敏感な検体について、値まで期待どおりであることをここで固定する。
TEST(EntityCommandPlayback, PlaysBackInNodeOrder)
{
	const std::array<nox::uint32, k_node_count> node_order{ 3u, 2u, 1u, 0u };
	const WorldSnapshot snapshot = RunWithNodeOrder(node_order);

	//	e0: node0 Add(10) → node1 Remove → node2 Add(20)。最後のAddが残る。
	EXPECT_TRUE(snapshot.entities[0].alive);
	EXPECT_TRUE(snapshot.entities[0].has_health);
	EXPECT_EQ(snapshot.entities[0].health_value, 20);

	//	e1: node0 Add(1) → node3 Remove。Removeが後なのでComponentDataは残らない。
	EXPECT_TRUE(snapshot.entities[1].alive);
	EXPECT_FALSE(snapshot.entities[1].has_health);

	//	e2: node1 Destroy。以降のAddは破棄済みentity宛なので捨てられる。
	EXPECT_FALSE(snapshot.entities[2].alive);
}

///	@brief	実スレッドで同時に記録しても、直列で記録したときと同じ結果になること。
///	@details	順列の総当たりが示すのは「記録順に依存しない」こと。
///				こちらは「実際に競争させても壊れない」ことを見る。
///				記録の重なりを増やすため複数回まわす。
TEST(EntityCommandPlayback, ConcurrentRecordingMatchesSerialRecording)
{
	const std::array<nox::uint32, k_node_count> node_order{ 0u, 1u, 2u, 3u };
	const WorldSnapshot expected = RunWithNodeOrder(node_order);

	constexpr nox::uint32 k_iteration_count = 32u;
	for (nox::uint32 iteration = 0u; iteration < k_iteration_count; ++iteration)
	{
		SCOPED_TRACE(::testing::Message() << "iteration = " << iteration);
		EXPECT_EQ(RunWithConcurrentNodes(), expected);
	}
}

//	=====================================================================================
//	2. ノードごとのバッファの独立性
//	=====================================================================================

///	@brief	あるバッファが溢れても、別のバッファの内容にも残容量にも影響しないこと。
///	@details	Worldを経由すると溢れは std::abort() でプロセスごと落ちるため、
///				記録が false を返すところまでをバッファ単体で見る。
TEST(EntityCommandPlayback, NodeBuffersAreIndependent)
{
	//	ペイロードは使わないので0でよい(TryDestroyはコマンド枠だけを消費する)。
	nox::EntityCommandBuffer<2u, 64u> node0;
	nox::EntityCommandBuffer<2u, 64u> node1;

	const nox::EntityId entity_a{ .generation = 1u, .index = 1u };
	const nox::EntityId entity_b{ .generation = 1u, .index = 2u };

	EXPECT_TRUE(node0.TryDestroy(entity_a));
	EXPECT_TRUE(node0.TryDestroy(entity_a));
	//	node0は満杯。
	EXPECT_FALSE(node0.TryDestroy(entity_a));
	EXPECT_EQ(node0.GetLength(), 2u);

	//	溢れた側の状態は、隣のバッファに一切伝播しない。
	EXPECT_EQ(node1.GetLength(), 0u);
	EXPECT_TRUE(node1.TryDestroy(entity_b));
	EXPECT_EQ(node1.GetLength(), 1u);

	nox::EntityCommand recorded{};
	ASSERT_TRUE(node1.TryGet(0u, recorded));
	EXPECT_EQ(recorded.entity_raw, entity_b.raw);

	//	片方のPlaybackはもう片方を空にしない。
	node1.BeginPlayback();
	node1.Clear();
	EXPECT_EQ(node1.GetLength(), 0u);
	EXPECT_EQ(node0.GetLength(), 2u);
}

//	=====================================================================================
//	3. high-water mark
//	=====================================================================================

///	@brief	high-water markがノード単位で取れること。
///	@details	容量は1本ごとに効くので、合計ではなく「1本あたりの最大値」が容量判断の指標になる。
TEST(EntityCommandPlayback, HighWaterMarkIsPerNode)
{
	nox::World world;
	world.ReserveNodeEntityCommandBuffers(k_node_count);
	ASSERT_EQ(world.GetNodeEntityCommandBufferCount(), k_node_count);

	std::array<nox::EntityId, k_entity_count> entities{};
	SetupEntities(world, entities);

	//	ノード0に3件、ノード2に1件、ノード外に2件を積み分ける。
	world.EnterEntityIteration();
	{
		const nox::WorldNodeCommandScope command_scope(world, 0u);
		nox::EntityCommands commands(world);
		commands.Add<PlaybackHealth>(entities[0], PlaybackHealth{ .value = 1 });
		commands.Add<PlaybackHealth>(entities[1], PlaybackHealth{ .value = 2 });
		commands.Remove<PlaybackHealth>(entities[1]);
	}
	{
		const nox::WorldNodeCommandScope command_scope(world, 2u);
		nox::EntityCommands commands(world);
		commands.Remove<PlaybackPosition>(entities[0]);
	}
	{
		//	スコープを張らずに積んだぶんは、ノード外バッファへ落ちる。
		nox::EntityCommands commands(world);
		commands.Add<PlaybackHealth>(entities[2], PlaybackHealth{ .value = 3 });
		commands.Remove<PlaybackPosition>(entities[2]);
	}
	world.LeaveEntityIteration();

	//	high-water markはPlaybackのClearで更新される。
	world.FlushEntityCommands();

	EXPECT_EQ(world.GetNodeEntityCommandPeakLength(0u), 3u);
	EXPECT_EQ(world.GetNodeEntityCommandPeakLength(1u), 0u);
	EXPECT_EQ(world.GetNodeEntityCommandPeakLength(2u), 1u);
	EXPECT_EQ(world.GetNodeEntityCommandPeakLength(3u), 0u);
	EXPECT_EQ(world.GetOutOfNodeEntityCommandPeakLength(), 2u);

	//	引数なしの版は「1本あたりの最大値のうち最大のもの」。合計(6)ではない。
	EXPECT_EQ(world.GetEntityCommandPeakLength(), 3u);

	//	ペイロードを運んだAddがあるノードだけ、ペイロードのhigh-water markが立つ。
	EXPECT_GT(world.GetNodeEntityCommandPeakPayloadLength(0u), 0u);
	EXPECT_EQ(world.GetNodeEntityCommandPeakPayloadLength(2u), 0u);

	//	未確保の番号を訊いても落ちない。
	EXPECT_EQ(world.GetNodeEntityCommandPeakLength(k_node_count), 0u);
}

//	=====================================================================================
//	4. 確保するバッファ本数が「宣言」で決まること
//	=====================================================================================

///	@brief	遅延構造変更を出しうるかが、引数リストから導出されていること。
///	@details	コマンドバッファを確保するのはこのフラグが真のノードだけなので、
///				フラグの導出が狂うと「積めるのにバッファが無い」か「無駄な空バッファ」になる。
///				購読済みの実型(生成コードが載せた表)に対して確認する。
TEST(EntityCommandPlayback, EmitsFlagComesFromTheArgumentList)
{
	//	テスト用System(TestMoveSystem / TestParallelAddSystem)はどちらも
	//	nox::EntityCommands& を宣言していないので、バッファは1本も要らない。
	nox::uint32 system_emitter_count = 0u;
	for (const nox::EntitySystemTypeDescriptor* const descriptor : nox::GetEntitySystemTypes())
	{
		ASSERT_NE(descriptor, nullptr);
		if (descriptor->emits_structural_change)
		{
			++system_emitter_count;
		}

		//	Chunk並列と構造変更の併記はコンパイルエラーになる(entity_system.hのstatic_assert)。
		//	表に載っている型が両方立てていないことを、実行時にも確認しておく。
		EXPECT_FALSE(descriptor->parallel_for_each && descriptor->emits_structural_change);
	}
	EXPECT_EQ(system_emitter_count, 0u);

	//	TestPlayerLogic は4メソッドのうち Process3 だけが nox::EntityCommands& を取る。
	nox::uint32 method_count = 0u;
	nox::uint32 method_emitter_count = 0u;
	for (const nox::EntityLogicTypeDescriptor* const descriptor : nox::GetEntityLogicTypes())
	{
		ASSERT_NE(descriptor, nullptr);
		for (const nox::EntityLogicMethodDescriptor& method : descriptor->get_methods())
		{
			++method_count;
			if (method.emits_structural_change)
			{
				++method_emitter_count;
			}
		}
	}

	//	「大半のノードは1バイトも要らない」ことが、この比で見える。
	EXPECT_GT(method_count, method_emitter_count);
	EXPECT_EQ(method_emitter_count, 1u);
	std::printf("nodes: systems=%u (emitters=%u), logic methods=%u (emitters=%u)\n",
		static_cast<nox::uint32>(nox::GetEntitySystemTypes().size()),
		system_emitter_count,
		method_count,
		method_emitter_count);
}

///	@brief	UpdaterGraphが、構造変更を出すノードにだけバッファ番号を振ること。
///	@details	確保本数はこの値で決まる。「ノード数ぶん確保していない」ことを直接確認する。
TEST(EntityCommandPlayback, GraphAssignsBufferIndicesOnlyToEmitters)
{
	//	購読済みの実型から、Worldと同じ手順でグラフを組む。
	nox::Vector<nox::EntitySystemBase*> systems;
	for (const nox::EntitySystemTypeDescriptor* const descriptor : nox::GetEntitySystemTypes())
	{
		systems.push_back(descriptor->create());
	}
	nox::Vector<nox::EntityLogicStorage*> storages;
	for (const nox::EntityLogicTypeDescriptor* const descriptor : nox::GetEntityLogicTypes())
	{
		storages.push_back(new nox::EntityLogicStorage(*descriptor));
	}

	{
		nox::UpdaterGraph graph;
		graph.Rebuild(
			std::span<nox::EntitySystemBase* const>(systems.data(), systems.size()),
			std::span<nox::EntityLogicStorage* const>(storages.data(), storages.size()));

		const std::span<const nox::UpdaterNode> nodes = graph.GetNodes(nox::SystemPhaseType::Update);
		const nox::uint32 buffer_count = graph.GetCommandBufferCount(nox::SystemPhaseType::Update);

		//	番号が振られたノードの数と、バッファ本数が一致すること。
		nox::uint32 indexed_count = 0u;
		nox::uint32 previous_index = 0u;
		bool ascending = true;
		for (const nox::UpdaterNode& node : nodes)
		{
			if (node.command_buffer_index == nox::k_invalid_updater_command_buffer_index)
			{
				continue;
			}

			//	番号は登録順に昇順で詰まっていること(=番号順の再生が登録順の再生と一致する)。
			if (indexed_count != 0u && node.command_buffer_index <= previous_index)
			{
				ascending = false;
			}
			previous_index = node.command_buffer_index;
			++indexed_count;
			EXPECT_LT(node.command_buffer_index, buffer_count);
		}

		EXPECT_TRUE(ascending);
		EXPECT_EQ(indexed_count, buffer_count);
		//	ノード数より確実に少ないこと。ここが今回の節約そのもの。
		EXPECT_LT(buffer_count, static_cast<nox::uint32>(nodes.size()));
		std::printf("Update phase: nodes=%zu, command buffers=%u\n", nodes.size(), buffer_count);
	}

	for (nox::EntityLogicStorage* const storage : storages)
	{
		delete storage;
	}
	for (nox::EntitySystemBase* const system : systems)
	{
		system->Destroy();
	}
}

///	@brief	World全体のメモリ量を記録に残す。
///	@details	ノード単位に分けたぶんの増減を、後から数字で追えるようにしておく。
///				コマンドバッファの実体はノード数ぶんヒープに載るので、
///				sizeof(nox::World) だけでは全体像にならない。両方を書き出す。
TEST(EntityCommandPlayback, ReportsMemoryFootprint)
{
	const size_t world_size = sizeof(nox::World);
	const nox::uint32 capacity = nox::World::GetEntityCommandCapacity();
	const nox::uint32 payload_capacity = nox::World::GetEntityCommandPayloadCapacity();

	std::printf("sizeof(nox::World) = %zu bytes\n", world_size);
	std::printf("EntityCommandBuffer capacity = %u commands / %u payload bytes\n", capacity, payload_capacity);

	//	バッファ1本の実バイト数。確保本数 x この値がヒープ側の総量になる。
	using BufferType = nox::EntityCommandBuffer<
		nox::World::GetEntityCommandCapacity(),
		nox::World::GetEntityCommandPayloadCapacity()>;
	std::printf("EntityCommandBuffer size = %zu bytes/buffer\n", sizeof(BufferType));

	EXPECT_GT(world_size, 0u);
	EXPECT_GT(capacity, 0u);
}
