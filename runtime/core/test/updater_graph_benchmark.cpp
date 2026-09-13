//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	updater_graph_benchmark.cpp
///	@brief	UpdaterGraph の2種類の並列化が実際に効くかの実測。assert は一切しない。
///	@details	SOL-AVES 式の並列化は独立した2つの機構で成り立っている。
///
///				  (a) ノードレベル並列 … 同一レイヤーの複数ノードをワーカーへ配る
///				                        (nox::World::ExecuteUpdaterGraphPhase)
///				  (b) チャンクレベル並列 … 1ノードの列挙を Chunk 単位でワーカーへ配る
///				                        (nox::World::ExecuteEntitySystemParallel、
///				                         System 側の k_parallel_for_each 宣言で有効化)
///
///				片方だけ効いていても全体の数字からは分からないので、両方を別々に測る。
///
///	@note		【CI では走らない】
///				全ケースが DISABLED_ 接頭辞付きで、既定の実行からは外れる。
///				実行時間はマシン負荷で揺れるため、タイミングに依存する assert を
///				CI へ入れない、という方針による。手で測るときだけ
///				  core_test.exe --gtest_also_run_disabled_tests --gtest_filter=UpdaterGraphBenchmark.*
///				で走らせる。結果は標準出力に表として出る。
///
///	@note		【本番経路との差】
///				本番(runtime.exe)側のワーカー数はコマンドラインで振れる
///				(--serial-updater / --updater-workers=N。nox::ResolveUpdaterWorkerCount)。
///				ただし nox::World::Init / ExecuteUpdaterGraphPhase / ExecuteEntitySystemParallel は
///				いずれも private で、core_test からフェーズ実行そのものを駆動できない。
///				そこでここでは自前の nox::JobSystem を持ち、world.cpp の2つの配り方を
///				同じ形で書き写している。測っているのは
///				  ・実物の nox::EntitySystem (nox::EntitySystemBase::Execute / ExecuteChunk)
///				  ・実物の nox::UpdaterGraph が算出したレイヤー分割
///				  ・実物の nox::JobSystem の配り・待ち
///				であり、写しているのは「レイヤーを回してジョブを積むループ」だけ。
///
///				本番の nox::World::ExecuteNode は非 Master でこれに加えて
///				宣言違反チェッカー (EnterNodeAccessScope / LeaveNodeAccessScope) と
///				コマンドバッファの束縛 (WorldNodeCommandScope) を通る。
///				どちらもノードあたり定数コストなので、ここの数字には含まれない。

#include	"pch.h"

#include	"../../kernel/kernel.h"
#include	"../../kernel/job_system.h"
#include	"../../kernel/stop_watch.h"
#include	"../../reflection/reflection.h"

#include	"../world.h"
#include	"../updater_graph.h"
#include	"../entity_system.h"

#include	<cmath>
#include	<cstdio>
#include	<thread>
#include	<vector>

namespace nox::test::bench
{
	//	=================================================================================
	//	ワークロード
	//	1エンティティあたりの仕事量は「軽すぎず、しかし現実的」を狙って超越関数を数回。
	//	軽すぎるとメモリ帯域だけを測ることになり、重すぎるとジョブの往復コストが埋もれて
	//	「並列化が効いた」と誤って読める。
	//	=================================================================================

	struct BenchPosition : nox::IComponentData { nox::float32 x; nox::float32 y; nox::float32 z; };
	struct BenchVelocity : nox::IComponentData { nox::float32 x; nox::float32 y; nox::float32 z; };
	struct BenchScalarA : nox::IComponentData { nox::float32 value; };
	struct BenchScalarB : nox::IComponentData { nox::float32 value; };
	struct BenchScalarC : nox::IComponentData { nox::float32 value; };
	struct BenchScalarD : nox::IComponentData { nox::float32 value; };

	/// @brief 1エンティティ分の計算。inline 展開されても消えない形にしてある。
	[[nodiscard]] inline nox::float32 BenchKernel(const nox::float32 seed, const nox::uint32 iteration_count)noexcept
	{
		nox::float32 value = seed;
		for (nox::uint32 i = 0u; i < iteration_count; ++i)
		{
			value = std::sinf(value) * 1.000001f + std::sqrtf(std::fabsf(value) + 1.0f);
		}
		return value;
	}

	/// @brief ノードレベル並列を測るための System 群。
	/// @details 4つとも触る ComponentData が完全に分かれているので、UpdaterGraph は
	///          4つ全部を同一レイヤーへ載せる。つまりレイヤー内ノード数が4になり、
	///          並列ディスパッチのコードパスが踏まれる。
	///          k_parallel_for_each は宣言しない (ノードレベルだけを測るため)。
	class BenchScalarASystem final : public nox::EntitySystem<nox::test::bench::BenchScalarASystem>
	{
	public:
		void OnUpdate(nox::test::bench::BenchScalarA& s) { s.value = BenchKernel(s.value, 24u); }
	};
	class BenchScalarBSystem final : public nox::EntitySystem<nox::test::bench::BenchScalarBSystem>
	{
	public:
		void OnUpdate(nox::test::bench::BenchScalarB& s) { s.value = BenchKernel(s.value, 24u); }
	};
	class BenchScalarCSystem final : public nox::EntitySystem<nox::test::bench::BenchScalarCSystem>
	{
	public:
		void OnUpdate(nox::test::bench::BenchScalarC& s) { s.value = BenchKernel(s.value, 24u); }
	};
	class BenchScalarDSystem final : public nox::EntitySystem<nox::test::bench::BenchScalarDSystem>
	{
	public:
		void OnUpdate(nox::test::bench::BenchScalarD& s) { s.value = BenchKernel(s.value, 24u); }
	};

	/// @brief チャンクレベル並列を測るための System。
	/// @details 宣言した ComponentData の自分の行しか触らないので k_parallel_for_each を宣言できる
	///          (条件は nox::IsParallelForEachEntitySystem のコメントを参照)。
	class BenchMoveSystem final : public nox::EntitySystem<nox::test::bench::BenchMoveSystem>
	{
	public:
		static constexpr bool k_parallel_for_each = true;

		void OnUpdate(nox::test::bench::BenchPosition& position, const nox::test::bench::BenchVelocity& velocity)
		{
			position.x = BenchKernel(position.x + velocity.x, 24u);
			position.y = BenchKernel(position.y + velocity.y, 24u);
		}
	};
}

namespace
{
	using namespace nox::test::bench;

	/// @brief 測定条件1本ぶんの結果。
	struct BenchResult final
	{
		nox::uint32 worker_count;
		double milliseconds;
	};

	//	---------------------------------------------------------------------------------
	//	world.cpp の配り方の写し
	//	---------------------------------------------------------------------------------

	struct NodeJobContext final
	{
		nox::World* world;
		const nox::UpdaterNode* node;
	};

	void ExecuteNodeJob(void* context)
	{
		auto* const job_context = static_cast<NodeJobContext*>(context);
		job_context->node->system->Execute(*job_context->world);
	}

	struct ChunkJobContext final
	{
		nox::World* world;
		nox::EntitySystemBase* system;
		nox::Archetype* archetype;
		nox::uint32 chunk_index;
	};

	void ExecuteChunkJob(void* context)
	{
		auto* const job_context = static_cast<ChunkJobContext*>(context);
		job_context->system->ExecuteChunk(*job_context->world, *job_context->archetype, job_context->chunk_index);
	}

	/// @brief nox::World::ExecuteUpdaterGraphPhase と同じ形でレイヤーを回す。
	void RunUpdaterGraphPhase(
		nox::World& world,
		nox::JobSystem& job_system,
		const nox::UpdaterGraph& graph,
		const nox::SystemPhaseType phase_type)
	{
		static constexpr nox::uint32 k_max_nodes_per_layer = 256u;

		const nox::uint32 layer_count = graph.GetLayerCount(phase_type);
		const bool parallel_enabled = (job_system.GetWorkerCount() != 0u);

		for (nox::uint32 layer_index = 0u; layer_index < layer_count; ++layer_index)
		{
			const std::span<const nox::UpdaterNode> nodes = graph.GetLayerNodes(phase_type, layer_index);

			//	1つしか無いレイヤーを配っても往復コストが乗るだけなので、その場で回す。
			if (parallel_enabled == false || nodes.size() <= 1u)
			{
				for (const nox::UpdaterNode& node : nodes)
				{
					node.system->Execute(world);
				}
				continue;
			}

			const nox::uint32 job_count = std::min(static_cast<nox::uint32>(nodes.size()), k_max_nodes_per_layer);
			std::array<NodeJobContext, k_max_nodes_per_layer> job_contexts{};
			std::array<nox::Job, k_max_nodes_per_layer> jobs{};
			for (nox::uint32 index = 0u; index < job_count; ++index)
			{
				job_contexts[index] = NodeJobContext{ .world = &world, .node = &nodes[index] };
				jobs[index] = nox::Job{ .func = &ExecuteNodeJob, .context = &job_contexts[index] };
			}

			nox::JobCounter counter{ 0u };
			job_system.Dispatch(std::span<const nox::Job>(jobs.data(), job_count), counter);
			job_system.Wait(counter);
		}
	}

	/// @brief nox::World::ExecuteEntitySystemParallel と同じ形で Chunk を配る。
	void RunEntitySystemParallel(
		nox::World& world,
		nox::JobSystem& job_system,
		nox::EntitySystemBase& system)
	{
		static constexpr nox::uint32 k_max_chunk_jobs_per_dispatch = 256u;

		const nox::EntityQuery& query = system.GetQuery();
		const nox::uint32 total_chunk_count = query.GetTotalChunkCount();

		if (total_chunk_count <= 1u || job_system.GetWorkerCount() == 0u)
		{
			system.Execute(world);
			return;
		}

		std::array<nox::EntityChunkRef, k_max_chunk_jobs_per_dispatch> chunk_refs{};
		std::array<ChunkJobContext, k_max_chunk_jobs_per_dispatch> job_contexts{};
		std::array<nox::Job, k_max_chunk_jobs_per_dispatch> jobs{};

		for (nox::uint32 start = 0u; start < total_chunk_count; start += k_max_chunk_jobs_per_dispatch)
		{
			const nox::uint32 job_count = query.FillChunkRefs(start, std::span<nox::EntityChunkRef>(chunk_refs));
			if (job_count == 0u) { break; }

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

	//	---------------------------------------------------------------------------------
	//	計測の共通部
	//	---------------------------------------------------------------------------------

	/// @brief 測るワーカー数の並び。0 = 直列 (スレッドを1本も作らない)。
	[[nodiscard]] std::vector<nox::uint32> MakeWorkerCounts()
	{
		const nox::uint32 hardware = std::max(1u, static_cast<nox::uint32>(std::thread::hardware_concurrency()));
		std::vector<nox::uint32> counts{ 0u, 1u, 2u, 4u };
		if (std::find(counts.begin(), counts.end(), hardware) == counts.end())
		{
			counts.push_back(hardware);
		}
		//	既定値 (論理プロセッサ数 - 1)。本番が実際に使う数。
		const nox::uint32 default_count = nox::JobSystem::GetDefaultWorkerCount();
		if (std::find(counts.begin(), counts.end(), default_count) == counts.end())
		{
			counts.push_back(default_count);
		}
		return counts;
	}

	/// @brief 中央値をとる。外れ値 (OS のスケジューリング揺れ) に引っ張られないため。
	[[nodiscard]] double Median(std::vector<double> samples)
	{
		std::sort(samples.begin(), samples.end());
		return samples[samples.size() / 2u];
	}

	void PrintTable(const char* title, const char* note, const std::vector<BenchResult>& results)
	{
		std::printf("\n=== %s ===\n%s\n", title, note);
		std::printf("%-10s | %12s | %12s | %8s\n", "workers", "median ms", "median us", "speedup");
		std::printf("-----------+--------------+--------------+---------\n");
		const double serial = results.empty() ? 0.0 : results.front().milliseconds;
		for (const BenchResult& result : results)
		{
			std::printf("%-10u | %12.3f | %12.2f | %7.2fx\n",
				result.worker_count,
				result.milliseconds,
				result.milliseconds * 1000.0,
				(result.milliseconds > 0.0) ? (serial / result.milliseconds) : 0.0);
		}
		std::printf("\n");
	}

	/// @brief ベンチ用の World をエンティティで埋める。
	/// @details 構造変更はフェーズ外なので即時系でよい。ここは測定対象ではない。
	void PopulateWorld(nox::World& world, const nox::uint32 entity_count)
	{
		for (nox::uint32 index = 0u; index < entity_count; ++index)
		{
			const nox::EntityId entity = world.CreateEntity();
			const nox::float32 seed = static_cast<nox::float32>(index % 97u) * 0.01f;

			world.AddComponent<BenchPosition>(entity)->x = seed;
			world.AddComponent<BenchVelocity>(entity)->x = 0.001f;
			world.AddComponent<BenchScalarA>(entity)->value = seed;
			world.AddComponent<BenchScalarB>(entity)->value = seed;
			world.AddComponent<BenchScalarC>(entity)->value = seed;
			world.AddComponent<BenchScalarD>(entity)->value = seed;
		}
	}
}

//	=====================================================================================
//	(a) ノードレベル並列
//	=====================================================================================

///	@brief	独立した4 System を1レイヤーへ載せ、ワーカー数を振って実測する。
TEST(UpdaterGraphBenchmark, DISABLED_NodeLevelParallel)
{
	static constexpr nox::uint32 k_entity_count = 20000u;
	static constexpr nox::uint32 k_warmup_frames = 3u;
	static constexpr nox::uint32 k_measure_frames = 15u;

	nox::World world;
	PopulateWorld(world, k_entity_count);

	BenchScalarASystem system_a;
	BenchScalarBSystem system_b;
	BenchScalarCSystem system_c;
	BenchScalarDSystem system_d;

	std::vector<nox::EntitySystemBase*> systems{ &system_a, &system_b, &system_c, &system_d };
	for (nox::EntitySystemBase* const system : systems)
	{
		world.BuildQuery(system->GetQuery(), system->GetDescriptor().make_read_write_mask());
	}

	nox::UpdaterGraph graph;
	graph.Rebuild(
		std::span<nox::EntitySystemBase* const>(systems.data(), systems.size()),
		std::span<nox::EntityLogicStorage* const>());

	//	前提の確認。1レイヤーに4ノードが載っていなければ、並列ディスパッチは踏まれない。
	const nox::uint32 layer_count = graph.GetLayerCount(nox::SystemPhaseType::Update);
	const size_t layer0_size = graph.GetLayerNodes(nox::SystemPhaseType::Update, 0u).size();
	std::printf("\n[NodeLevel] entities=%u layers=%u layer0_nodes=%zu\n",
		k_entity_count, layer_count, layer0_size);
	ASSERT_EQ(layer_count, 1u);
	ASSERT_EQ(layer0_size, 4u);

	std::vector<BenchResult> results;
	for (const nox::uint32 worker_count : MakeWorkerCounts())
	{
		nox::JobSystem job_system;
		job_system.Initialize(worker_count);

		for (nox::uint32 i = 0u; i < k_warmup_frames; ++i)
		{
			RunUpdaterGraphPhase(world, job_system, graph, nox::SystemPhaseType::Update);
		}

		std::vector<double> samples;
		samples.reserve(k_measure_frames);
		for (nox::uint32 i = 0u; i < k_measure_frames; ++i)
		{
			nox::StopWatch watch;
			watch.Start();
			RunUpdaterGraphPhase(world, job_system, graph, nox::SystemPhaseType::Update);
			watch.Stop();
			samples.push_back(static_cast<double>(watch.ElapsedMilliseconds()));
		}

		job_system.Finalize();
		results.push_back(BenchResult{ .worker_count = worker_count, .milliseconds = Median(samples) });
	}

	PrintTable("Node-level parallel (4 independent systems in one layer)",
		"1レイヤー = 4ノード。ノードごとに1ジョブ。並列度の上限は4。\n"
		"待つ側(配った本人)も自分でジョブを引くので、実効並列度は workers + 1。",
		results);
}

//	=====================================================================================
//	(b) チャンクレベル並列
//	=====================================================================================

///	@brief	k_parallel_for_each を宣言した1 System の列挙を Chunk 単位で配り、実測する。
TEST(UpdaterGraphBenchmark, DISABLED_ChunkLevelParallel)
{
	static constexpr nox::uint32 k_entity_count = 20000u;
	static constexpr nox::uint32 k_warmup_frames = 3u;
	static constexpr nox::uint32 k_measure_frames = 15u;

	nox::World world;
	PopulateWorld(world, k_entity_count);

	BenchMoveSystem system;
	world.BuildQuery(system.GetQuery(), system.GetDescriptor().make_read_write_mask());

	const nox::uint32 chunk_count = system.GetQuery().GetTotalChunkCount();
	std::printf("\n[ChunkLevel] entities=%u chunks=%u parallel_for_each=%d\n",
		k_entity_count, chunk_count, system.GetDescriptor().parallel_for_each ? 1 : 0);
	ASSERT_TRUE(system.GetDescriptor().parallel_for_each);

	std::vector<BenchResult> results;
	for (const nox::uint32 worker_count : MakeWorkerCounts())
	{
		nox::JobSystem job_system;
		job_system.Initialize(worker_count);

		for (nox::uint32 i = 0u; i < k_warmup_frames; ++i)
		{
			RunEntitySystemParallel(world, job_system, system);
		}

		std::vector<double> samples;
		samples.reserve(k_measure_frames);
		for (nox::uint32 i = 0u; i < k_measure_frames; ++i)
		{
			nox::StopWatch watch;
			watch.Start();
			RunEntitySystemParallel(world, job_system, system);
			watch.Stop();
			samples.push_back(static_cast<double>(watch.ElapsedMilliseconds()));
		}

		job_system.Finalize();
		results.push_back(BenchResult{ .worker_count = worker_count, .milliseconds = Median(samples) });
	}

	PrintTable("Chunk-level parallel (k_parallel_for_each, single system)",
		"1ノードの列挙を Chunk 単位で配る。並列度の上限は Chunk 数。\n"
		"待つ側(配った本人)も自分でジョブを引くので、実効並列度は workers + 1。",
		results);
}

///	@brief	ジョブの往復コストそのもの。ワークロードを空にして配り・待ちだけを測る。
///	@details	上の2つで「速くならなかった」ときに、原因がジョブの往復コストなのか
///				ワークロードの粒度なのかを切り分けるための対照。
TEST(UpdaterGraphBenchmark, DISABLED_DispatchOverhead)
{
	static constexpr nox::uint32 k_dispatch_count = 1000u;
	static constexpr nox::uint32 k_job_count = 4u;

	std::vector<BenchResult> results;
	for (const nox::uint32 worker_count : MakeWorkerCounts())
	{
		nox::JobSystem job_system;
		job_system.Initialize(worker_count);

		std::array<nox::Job, k_job_count> jobs{};
		for (nox::uint32 index = 0u; index < k_job_count; ++index)
		{
			jobs[index] = nox::Job{ .func = [](void*)noexcept {}, .context = nullptr };
		}

		//	ウォームアップ
		for (nox::uint32 i = 0u; i < 100u; ++i)
		{
			nox::JobCounter counter{ 0u };
			job_system.Dispatch(std::span<const nox::Job>(jobs.data(), jobs.size()), counter);
			job_system.Wait(counter);
		}

		std::vector<double> samples;
		for (nox::uint32 repeat = 0u; repeat < 5u; ++repeat)
		{
			nox::StopWatch watch;
			watch.Start();
			for (nox::uint32 i = 0u; i < k_dispatch_count; ++i)
			{
				nox::JobCounter counter{ 0u };
				job_system.Dispatch(std::span<const nox::Job>(jobs.data(), jobs.size()), counter);
				job_system.Wait(counter);
			}
			watch.Stop();
			samples.push_back(static_cast<double>(watch.ElapsedMilliseconds()) / static_cast<double>(k_dispatch_count));
		}

		job_system.Finalize();
		results.push_back(BenchResult{ .worker_count = worker_count, .milliseconds = Median(samples) });
	}

	PrintTable("Dispatch overhead (4 empty jobs per dispatch)",
		"1 Dispatch + Wait あたりのミリ秒。ワークロードは空。speedup 列は意味を持たない。",
		results);
}
