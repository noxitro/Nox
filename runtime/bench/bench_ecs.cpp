//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	bench_ecs.cpp
///	@brief	ECS (World / EntitySystem / EntityCommands / UpdaterGraph) のベンチマーク
///	@details	ここの ComponentData / EntitySystem はこのファイルだけの型で、リフレクション生成を通さない。
///				ComponentData の型番号は初回使用時に実行時で振られ、System の記述子は constexpr で
///				このファイル内に実体化されるので、生成コードが無くても動く
///				(core/test/updater_graph_benchmark.cpp と同じ作り)。
///				そのかわり World::Init が System を自動生成することはないので、System は手で作って
///				BuildQuery を呼ぶ。World::Init / ExecuteUpdaterGraphPhase は private なので、
///				並列列挙は world.cpp の配り方を写した関数で回す。
///
///				本番の World::ExecuteNode が非 Master で通す宣言違反チェッカーと
///				コマンドバッファの束縛はノードあたり定数コストなので、ここの数字には含まれない。

#include	"pch.h"
#include	"bench.h"

#include	"core/world.h"
#include	"core/entity_system.h"
#include	"core/entity_query.h"
#include	"core/entity_commands.h"
#include	"core/updater_graph.h"

#include	<array>
#include	<span>
#include	<utility>
#include	<vector>

namespace nox::bench::ecs
{
	struct BPosition : nox::IComponentData { nox::float32 x; nox::float32 y; nox::float32 z; };
	struct BVelocity : nox::IComponentData { nox::float32 x; nox::float32 y; nox::float32 z; };

	//	Archetype を割るためだけの 1 バイトのタグ
	struct BTag0 : nox::IComponentData { nox::uint8 value; };
	struct BTag1 : nox::IComponentData { nox::uint8 value; };
	struct BTag2 : nox::IComponentData { nox::uint8 value; };
	struct BTag3 : nox::IComponentData { nox::uint8 value; };

	//	UpdaterGraph のレイヤー分割用
	struct BL0 : nox::IComponentData { nox::float32 value; };
	struct BL1 : nox::IComponentData { nox::float32 value; };
	struct BL2 : nox::IComponentData { nox::float32 value; };
	struct BL3 : nox::IComponentData { nox::float32 value; };
	struct BL4 : nox::IComponentData { nox::float32 value; };
	struct BL5 : nox::IComponentData { nox::float32 value; };
	struct BL6 : nox::IComponentData { nox::float32 value; };
	struct BL7 : nox::IComponentData { nox::float32 value; };

	/// @brief 位置 += 速度 (直列)
	class BMoveSystem final : public nox::EntitySystem<nox::bench::ecs::BMoveSystem>
	{
	public:
		void OnUpdate(nox::bench::ecs::BPosition& position, const nox::bench::ecs::BVelocity& velocity)
		{
			position.x += velocity.x;
			position.y += velocity.y;
			position.z += velocity.z;
		}
	};

	/// @brief 位置 += 速度 (Chunk 単位の並列列挙を許す)
	class BMoveParallelSystem final : public nox::EntitySystem<nox::bench::ecs::BMoveParallelSystem>
	{
	public:
		//	この名前はエンジンが読む固定名 (nox::IsParallelForEachEntitySystem)。kPascalCase へ変えないこと
		static constexpr bool k_parallel_for_each = true;

		void OnUpdate(nox::bench::ecs::BPosition& position, const nox::bench::ecs::BVelocity& velocity)
		{
			position.x += velocity.x;
			position.y += velocity.y;
			position.z += velocity.z;
		}
	};

	//	UpdaterGraph::Rebuild 用の 8 個。書き込み 4 個 → 2 段の合流 → 最終段で 4 レイヤーになる。
	//	データの流れ (書き込み → 合流 → 最終段) は RunAfter で宣言する。宣言しないと全順序が型名順
	//	(BLFinal < BLMerge.. < BLWrite..) になり、流れと逆向きに直列化されて 2 レイヤーに潰れる。
	class BLWrite0System final : public nox::EntitySystem<nox::bench::ecs::BLWrite0System>
	{
	public:
		void OnUpdate(nox::bench::ecs::BL0& value) { value.value += 1.0f; }
	};
	class BLWrite1System final : public nox::EntitySystem<nox::bench::ecs::BLWrite1System>
	{
	public:
		void OnUpdate(nox::bench::ecs::BL1& value) { value.value += 1.0f; }
	};
	class BLWrite2System final : public nox::EntitySystem<nox::bench::ecs::BLWrite2System>
	{
	public:
		void OnUpdate(nox::bench::ecs::BL2& value) { value.value += 1.0f; }
	};
	class BLWrite3System final : public nox::EntitySystem<nox::bench::ecs::BLWrite3System>
	{
	public:
		void OnUpdate(nox::bench::ecs::BL3& value) { value.value += 1.0f; }
	};
	class BLMerge01System final : public nox::EntitySystem<nox::bench::ecs::BLMerge01System>
	{
	public:
		using RunAfter = nox::TypeList<nox::bench::ecs::BLWrite0System, nox::bench::ecs::BLWrite1System>;
		void OnUpdate(nox::bench::ecs::BL4& out, const nox::bench::ecs::BL0& a, const nox::bench::ecs::BL1& b) { out.value = a.value + b.value; }
	};
	class BLMerge23System final : public nox::EntitySystem<nox::bench::ecs::BLMerge23System>
	{
	public:
		using RunAfter = nox::TypeList<nox::bench::ecs::BLWrite2System, nox::bench::ecs::BLWrite3System>;
		void OnUpdate(nox::bench::ecs::BL5& out, const nox::bench::ecs::BL2& a, const nox::bench::ecs::BL3& b) { out.value = a.value + b.value; }
	};
	class BLMerge45System final : public nox::EntitySystem<nox::bench::ecs::BLMerge45System>
	{
	public:
		using RunAfter = nox::TypeList<nox::bench::ecs::BLMerge01System, nox::bench::ecs::BLMerge23System>;
		void OnUpdate(nox::bench::ecs::BL6& out, const nox::bench::ecs::BL4& a, const nox::bench::ecs::BL5& b) { out.value = a.value + b.value; }
	};
	class BLFinalSystem final : public nox::EntitySystem<nox::bench::ecs::BLFinalSystem>
	{
	public:
		using RunAfter = nox::TypeList<nox::bench::ecs::BLMerge45System>;
		void OnUpdate(nox::bench::ecs::BL7& out, const nox::bench::ecs::BL6& a) { out.value = a.value * 0.5f; }
	};
}

namespace
{
	using nox::bench::ecs::BPosition;
	using nox::bench::ecs::BVelocity;

	/// @brief {BPosition, BVelocity} を entity_count 体作る
	/// @details tag_bit_count 本のタグで 2^tag_bit_count 個の Archetype へ割る。
	///          Query は作成済みの Archetype しか拾わないので、BuildQuery はこの後に呼ぶこと。
	void PopulateMovers(
		nox::World& world,
		const nox::uint32 entity_count,
		const nox::uint32 tag_bit_count,
		nox::EntityId* const out_entities)
	{
		const std::array<const nox::ComponentTypeInfo*, 4> tag_types{
			&nox::ComponentTypeOf<nox::bench::ecs::BTag0>(),
			&nox::ComponentTypeOf<nox::bench::ecs::BTag1>(),
			&nox::ComponentTypeOf<nox::bench::ecs::BTag2>(),
			&nox::ComponentTypeOf<nox::bench::ecs::BTag3>(),
		};
		for (nox::uint32 index = 0u; index < entity_count; ++index)
		{
			const nox::EntityId entity = world.CreateEntity();
			world.AddComponent<BPosition>(entity)->x = static_cast<nox::float32>(index % 97u) * 0.01f;
			world.AddComponent<BVelocity>(entity)->x = 0.001f;
			for (nox::uint32 bit = 0u; bit < tag_bit_count && bit < tag_types.size(); ++bit)
			{
				if (((index >> bit) & 1u) != 0u)
				{
					world.AddComponent(entity, *tag_types[bit]);
				}
			}
			if (out_entities != nullptr)
			{
				out_entities[index] = entity;
			}
		}
	}

	//	---------------------------------------------------------------------------------
	//	列挙
	//	---------------------------------------------------------------------------------

	void RunQueryIterate(nox::bench::State& state, const nox::uint32 tag_bit_count)
	{
		static constexpr nox::uint32 kEntityCount = 10000u;

		nox::World world;
		PopulateMovers(world, kEntityCount, tag_bit_count, nullptr);

		nox::bench::ecs::BMoveSystem system;
		world.BuildQuery(system.GetQuery(), system.GetDescriptor().make_read_write_mask());

		state.SetItemsPerOp(kEntityCount);
		state.Run([&world, &system](const nox::uint64 op_count)
			{
				for (nox::uint64 op = 0u; op < op_count; ++op)
				{
					system.Execute(world);
				}
			});
	}

	void BenchQueryIterate10k(nox::bench::State& state)
	{
		RunQueryIterate(state, 0u);
	}

	void BenchQueryIterateFrag16(nox::bench::State& state)
	{
		RunQueryIterate(state, 4u);
	}

	//	world.cpp (nox::World::ExecuteEntitySystemParallel) の配り方の写し
	struct ChunkJobContext final
	{
		nox::World* world;
		nox::EntitySystemBase* system;
		nox::Archetype* archetype;
		nox::uint32 chunk_index;
	};

	void ExecuteChunkJob(void* const context)
	{
		ChunkJobContext* const job_context = static_cast<ChunkJobContext*>(context);
		job_context->system->ExecuteChunk(*job_context->world, *job_context->archetype, job_context->chunk_index);
	}

	void RunEntitySystemParallel(nox::World& world, nox::JobSystem& job_system, nox::EntitySystemBase& system)
	{
		static constexpr nox::uint32 kMaxChunkJobsPerDispatch = 256u;

		const nox::EntityQuery& query = system.GetQuery();
		const nox::uint32 total_chunk_count = query.GetTotalChunkCount();
		if (total_chunk_count <= 1u || job_system.GetWorkerCount() == 0u)
		{
			system.Execute(world);
			return;
		}

		std::array<nox::EntityChunkRef, kMaxChunkJobsPerDispatch> chunk_refs{};
		std::array<ChunkJobContext, kMaxChunkJobsPerDispatch> job_contexts{};
		std::array<nox::Job, kMaxChunkJobsPerDispatch> jobs{};
		for (nox::uint32 start = 0u; start < total_chunk_count; start += kMaxChunkJobsPerDispatch)
		{
			const nox::uint32 job_count = query.FillChunkRefs(start, std::span<nox::EntityChunkRef>(chunk_refs));
			if (job_count == 0u)
			{
				break;
			}
			for (nox::uint32 index = 0u; index < job_count; ++index)
			{
				job_contexts[index] = ChunkJobContext{
					.world = &world,
					.system = &system,
					.archetype = chunk_refs[index].archetype,
					.chunk_index = chunk_refs[index].chunk_index,
				};
				jobs[index] = nox::Job{ .func = &ExecuteChunkJob, .context = &job_contexts[index] };
			}

			nox::JobCounter counter{ 0u };
			job_system.Dispatch(std::span<const nox::Job>(jobs.data(), job_count), counter);
			job_system.Wait(counter);
		}
	}

	/// @brief 10 万体を Chunk 単位でワーカーへ配って列挙する
	void BenchQueryParallel100k(nox::bench::State& state)
	{
		static constexpr nox::uint32 kEntityCount = 100000u;

		nox::World world;
		PopulateMovers(world, kEntityCount, 0u, nullptr);

		nox::bench::ecs::BMoveParallelSystem system;
		world.BuildQuery(system.GetQuery(), system.GetDescriptor().make_read_write_mask());

		nox::JobSystem job_system;
		job_system.Initialize(nox::JobSystem::GetDefaultWorkerCount());
		state.SetThreads(job_system.GetWorkerCount() + 1u);
		state.SetItemsPerOp(kEntityCount);

		state.Run([&world, &job_system, &system](const nox::uint64 op_count)
			{
				for (nox::uint64 op = 0u; op < op_count; ++op)
				{
					RunEntitySystemParallel(world, job_system, system);
				}
			});

		job_system.Finalize();
	}

	//	---------------------------------------------------------------------------------
	//	ランダムアクセス
	//	---------------------------------------------------------------------------------

	/// @brief TryGetComponent をシャッフルした順で 1 万回 (EntityLogic やゲームロジックの引き方)
	void BenchGetComponentRandom10k(nox::bench::State& state)
	{
		static constexpr nox::uint32 kEntityCount = 10000u;

		nox::World world;
		std::vector<nox::EntityId> order(kEntityCount);
		PopulateMovers(world, kEntityCount, 2u, order.data());

		//	xorshift64 で決定的に並べ替える (実行ごと・A/B で同じ順になる)
		nox::uint64 random_state = 0x2545F4914F6CDD1Dull;
		for (size_t index = order.size(); index > 1u; --index)
		{
			random_state ^= random_state << 13u;
			random_state ^= random_state >> 7u;
			random_state ^= random_state << 17u;
			std::swap(order[index - 1u], order[static_cast<size_t>(random_state % index)]);
		}

		state.SetItemsPerOp(kEntityCount);
		state.Run([&world, &order](const nox::uint64 op_count)
			{
				nox::float32 sum = 0.0f;
				for (nox::uint64 op = 0u; op < op_count; ++op)
				{
					for (const nox::EntityId entity : order)
					{
						const BPosition* const position = world.TryGetComponent<BPosition>(entity);
						sum += (position != nullptr) ? position->x : 0.0f;
					}
				}
				nox::bench::DoNotOptimize(sum);
			});
	}

	//	---------------------------------------------------------------------------------
	//	構造変更
	//	---------------------------------------------------------------------------------

	constexpr nox::uint32 kStructuralEntityCount = 1024u;

	/// @brief 1024 体にタグを Add → Remove (Archetype 間の移動 2 回ずつ)
	void BenchTagToggle1k(nox::bench::State& state)
	{
		nox::World world;
		std::vector<nox::EntityId> entities(kStructuralEntityCount);
		PopulateMovers(world, kStructuralEntityCount, 0u, entities.data());

		//	移動先の Archetype を先に作っておく (初回の Archetype 生成を計測から外す)
		world.AddComponent<nox::bench::ecs::BTag0>(entities[0]);
		world.RemoveComponent<nox::bench::ecs::BTag0>(entities[0]);

		state.SetItemsPerOp(kStructuralEntityCount);
		state.Run([&world, &entities](const nox::uint64 op_count)
			{
				for (nox::uint64 op = 0u; op < op_count; ++op)
				{
					for (const nox::EntityId entity : entities)
					{
						world.AddComponent<nox::bench::ecs::BTag0>(entity);
					}
					for (const nox::EntityId entity : entities)
					{
						world.RemoveComponent<nox::bench::ecs::BTag0>(entity);
					}
				}
			});
	}

	/// @brief 即時 API で 1024 体を生成 + 2 コンポーネント追加し、全部破棄する
	void BenchSpawnDespawn1k(nox::bench::State& state)
	{
		nox::World world;
		std::vector<nox::EntityId> entities(kStructuralEntityCount);

		state.SetItemsPerOp(kStructuralEntityCount);
		state.Run([&world, &entities](const nox::uint64 op_count)
			{
				for (nox::uint64 op = 0u; op < op_count; ++op)
				{
					for (nox::uint32 index = 0u; index < kStructuralEntityCount; ++index)
					{
						const nox::EntityId entity = world.CreateEntity();
						world.AddComponent<BPosition>(entity)->x = 1.0f;
						world.AddComponent<BVelocity>(entity)->x = 0.1f;
						entities[index] = entity;
					}
					for (const nox::EntityId entity : entities)
					{
						world.DestroyEntity(entity);
					}
				}
			});
	}

	/// @brief EntityCommands で 1024 体の生成を記録 → Flush → 破棄を記録 → Flush
	/// @details フェーズ中の生成経路。フェーズの印は外から立てられないので、
	///          テスト (entity_command_playback_test.cpp) と同じく EnterEntityIteration で代用する。
	void BenchDeferredSpawn1k(nox::bench::State& state)
	{
		static constexpr nox::uint32 kNodeCount = 4u;
		static constexpr nox::uint32 kSpawnsPerNode = kStructuralEntityCount / kNodeCount;
		//	1 体あたり Add 2 件。1 ノードのバッファ容量を超えると std::abort になる
		static_assert(kSpawnsPerNode * 2u <= nox::World::GetEntityCommandCapacity());

		nox::World world;
		world.ReserveNodeEntityCommandBuffers(kNodeCount);
		std::vector<nox::EntityId> spawned(kStructuralEntityCount);

		state.SetItemsPerOp(kStructuralEntityCount);
		state.Run([&world, &spawned](const nox::uint64 op_count)
			{
				for (nox::uint64 op = 0u; op < op_count; ++op)
				{
					world.EnterEntityIteration();
					for (nox::uint32 node = 0u; node < kNodeCount; ++node)
					{
						const nox::WorldNodeCommandScope command_scope(world, node);
						nox::EntityCommands commands(world);
						for (nox::uint32 index = 0u; index < kSpawnsPerNode; ++index)
						{
							const nox::EntityId entity = commands.Create();
							commands.Add<BPosition>(entity, BPosition{ .x = 1.0f, .y = 0.0f, .z = 0.0f });
							commands.Add<BVelocity>(entity, BVelocity{ .x = 0.1f, .y = 0.0f, .z = 0.0f });
							spawned[(node * kSpawnsPerNode) + index] = entity;
						}
					}
					world.LeaveEntityIteration();
					//	列挙を閉じてから反映する (開いたままだと std::abort)
					world.FlushEntityCommands();

					world.EnterEntityIteration();
					for (nox::uint32 node = 0u; node < kNodeCount; ++node)
					{
						const nox::WorldNodeCommandScope command_scope(world, node);
						nox::EntityCommands commands(world);
						for (nox::uint32 index = 0u; index < kSpawnsPerNode; ++index)
						{
							commands.Destroy(spawned[(node * kSpawnsPerNode) + index]);
						}
					}
					world.LeaveEntityIteration();
					world.FlushEntityCommands();
				}
			});
	}

	//	---------------------------------------------------------------------------------
	//	UpdaterGraph
	//	---------------------------------------------------------------------------------

	/// @brief 64 ノードのレイヤー分割 (nox::BuildUpdaterLayerIndices。ヒープを使わない純粋関数)
	void BenchLayerIndices64(nox::bench::State& state)
	{
		static constexpr nox::uint32 kNodeCount = 64u;

		const std::array<nox::ComponentMask, 8> masks{
			nox::MakeComponentMask<nox::bench::ecs::BL0>(),
			nox::MakeComponentMask<nox::bench::ecs::BL1>(),
			nox::MakeComponentMask<nox::bench::ecs::BL2>(),
			nox::MakeComponentMask<nox::bench::ecs::BL3>(),
			nox::MakeComponentMask<nox::bench::ecs::BL4>(),
			nox::MakeComponentMask<nox::bench::ecs::BL5>(),
			nox::MakeComponentMask<nox::bench::ecs::BL6>(),
			nox::MakeComponentMask<nox::bench::ecs::BL7>(),
		};

		//	2 種類を読み書きし、4 つに 1 つは片方へ書き込む宣言を決定的に作る
		std::array<nox::UpdaterNodeAccess, kNodeCount> accesses{};
		nox::uint64 random_state = 0x9E3779B97F4A7C15ull;
		for (nox::UpdaterNodeAccess& access : accesses)
		{
			random_state ^= random_state << 13u;
			random_state ^= random_state >> 7u;
			random_state ^= random_state << 17u;
			const nox::ComponentMask& first = masks[static_cast<size_t>(random_state % masks.size())];
			const nox::ComponentMask& second = masks[static_cast<size_t>((random_state >> 8u) % masks.size())];
			access.read_write_mask = first;
			access.read_write_mask.Merge(second);
			access.write_mask = (((random_state >> 16u) & 3u) == 0u) ? first : nox::ComponentMask{};
		}

		std::array<nox::uint32, kNodeCount> layers{};
		state.Run([&accesses, &layers](const nox::uint64 op_count)
			{
				for (nox::uint64 op = 0u; op < op_count; ++op)
				{
					nox::bench::DoNotOptimize(accesses);
					const nox::uint32 layer_count = nox::BuildUpdaterLayerIndices(
						std::span<const nox::UpdaterNodeAccess>(accesses.data(), accesses.size()),
						std::span<nox::uint32>(layers.data(), layers.size()));
					nox::bench::DoNotOptimize(layer_count);
				}
			});
	}

	/// @brief System 8 個から UpdaterGraph を組み直す (起動時・ホットリロード時のコスト)
	void BenchGraphRebuild8(nox::bench::State& state)
	{
		nox::bench::ecs::BLWrite0System write0;
		nox::bench::ecs::BLWrite1System write1;
		nox::bench::ecs::BLWrite2System write2;
		nox::bench::ecs::BLWrite3System write3;
		nox::bench::ecs::BLMerge01System merge01;
		nox::bench::ecs::BLMerge23System merge23;
		nox::bench::ecs::BLMerge45System merge45;
		nox::bench::ecs::BLFinalSystem final_system;
		const std::array<nox::EntitySystemBase*, 8> systems{
			&write0, &write1, &write2, &write3, &merge01, &merge23, &merge45, &final_system,
		};

		nox::UpdaterGraph graph;
		state.Run([&graph, &systems](const nox::uint64 op_count)
			{
				for (nox::uint64 op = 0u; op < op_count; ++op)
				{
					graph.Rebuild(
						std::span<nox::EntitySystemBase* const>(systems.data(), systems.size()),
						std::span<nox::EntityLogicStorage* const>());
				}
				nox::bench::DoNotOptimize(graph);
			});
	}
}

std::span<const nox::bench::Definition> nox::bench::GetEcsBenchmarks()noexcept
{
	static constexpr std::array<nox::bench::Definition, 9> kDefinitions{ {
		{ .name = "ecs/query_iterate/10k", .title = "EntitySystem::Execute で 1 万体を列挙 (Archetype 1 個)", .per = "entity", .alloc_budget = 0, .function = &BenchQueryIterate10k },
		{ .name = "ecs/query_iterate_frag16/10k", .title = "1 万体を列挙 (Archetype 16 個に分散)", .per = "entity", .alloc_budget = 0, .function = &BenchQueryIterateFrag16 },
		{ .name = "ecs/query_parallel/100k", .title = "Chunk 単位の並列列挙 (10 万体, 既定ワーカー数)", .per = "entity", .alloc_budget = 0, .function = &BenchQueryParallel100k },
		{ .name = "ecs/get_component_random/10k", .title = "TryGetComponent のランダムアクセス (1 万体)", .per = "lookup", .alloc_budget = 0, .function = &BenchGetComponentRandom10k },
		{ .name = "ecs/tag_toggle/1k", .title = "タグ Add→Remove による Archetype 移動 (1024 体)", .per = "entity", .alloc_budget = nox::bench::kNoBudget, .function = &BenchTagToggle1k },
		{ .name = "ecs/spawn_despawn/1k", .title = "即時 API で生成 + 2 コンポーネント追加 + 破棄 (1024 体)", .per = "entity", .alloc_budget = nox::bench::kNoBudget, .function = &BenchSpawnDespawn1k },
		{ .name = "ecs/deferred_spawn/1k", .title = "EntityCommands で生成を記録 → Flush → 破棄 (1024 体)", .per = "entity", .alloc_budget = nox::bench::kNoBudget, .function = &BenchDeferredSpawn1k },
		{ .name = "ecs/layer_indices/64", .title = "UpdaterGraph のレイヤー分割 (64 ノード)", .per = "call", .alloc_budget = 0, .function = &BenchLayerIndices64 },
		{ .name = "ecs/graph_rebuild/8", .title = "UpdaterGraph::Rebuild (System 8 個)", .per = "call", .alloc_budget = nox::bench::kNoBudget, .function = &BenchGraphRebuild8 },
	} };
	return kDefinitions;
}
