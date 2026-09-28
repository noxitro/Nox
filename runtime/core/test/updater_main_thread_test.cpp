//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	updater_main_thread_test.cpp
///	@brief	kMainThreadOnly を宣言したノードが、ワーカーへ配られず呼び出しスレッド上で走ることの検証。
///	@details	宣言(型の `static constexpr bool kMainThreadOnly = true;`)から実行までを3段で見る。
///
///				1. 記述子 … EntitySystem / EntityLogic の記述子に main_thread_only が載る
///				2. グラフ … nox::UpdaterGraph が UpdaterNode::main_thread_only へ写す
///				            (EntityLogic は型の全更新メソッドに掛かる)
///				3. 実行   … nox::ExecuteUpdaterLayer が main_thread_only のノードを呼び出しスレッドで回し、
///				            それ以外をワーカーへ配る
///
///	@note		World は組み立てるが、フェーズ実行は World を通さない。
///				nox::World::Init / ExecuteUpdaterGraphPhase は private で、ワーカー数もプロセスの
///				コマンドラインで決まる(nox::ResolveUpdaterWorkerCount)ため、core_test から
///				「ワーカー数 >= 1 の World」を作れない。そこで World が各レイヤーで呼んでいる
///				nox::ExecuteUpdaterLayer を、自前の nox::JobSystem でそのまま呼ぶ。
///				写しではなく本物の配分点を通すので、World 側の経路と同じものを検証している。
///
///	@note		対照(kMainThreadOnly の無いノードが別スレッドでも走ること)はスケジューリングに依存しうるので、
///				フレーキーにならない形にしてある。対照ノードは「どれか1つが呼び出しスレッド以外で走る」まで
///				待ってから戻る。呼び出しスレッドは同時に1つのジョブしか抱えられないので、
///				対照ノードが2つ以上あれば、残りは必ずワーカーが引く。ワーカーが一切動かない場合だけ
///				上限時間で抜けて失敗する(= 本当に並列に配られていない)。

#include	"pch.h"

//	core_test の pch.h は gtest しか載せていない (test_new_delete.cpp の都合)。
//	core のヘッダは kernel / reflection の基盤型に依存するので、main.cpp と同じ順で先に入れる。
#include	"../../kernel/kernel.h"
#include	"../../kernel/job_system.h"
#include	"../../reflection/reflection.h"

#include	"../world.h"
#include	"../updater_graph.h"
#include	"../entity_system.h"
#include	"../entity_logic.h"

#include	<atomic>
#include	<chrono>
#include	<thread>
#include	<vector>

namespace nox::test::main_thread
{
	//	=================================================================================
	//	スレッドの記録
	//	=================================================================================

	/// @brief 呼び出しスレッド(= フェーズを回しているスレッド)の ID。テストの先頭で入れる。
	/// @details Dispatch のキュー操作(ミューテックス)を挟んで読まれるので、ワーカーからも見える。
	inline std::thread::id g_caller_thread_id{};

	/// @brief 対照ノードのどれかが呼び出しスレッド以外で走ったか。
	inline std::atomic<bool> g_control_ran_off_caller{ false };

	/// @brief 対照ノードが「呼び出しスレッド以外で走った」を待つ上限。これを超えたら諦めて戻る(= 失敗)。
	inline constexpr std::chrono::seconds kControlWaitLimit{ 10 };

	/// @brief 対照ノードの本体。呼び出しスレッド以外で走ったら印を付け、印が付くまで待つ。
	inline void RunControlNode()noexcept
	{
		if (std::this_thread::get_id() != g_caller_thread_id)
		{
			g_control_ran_off_caller.store(true, std::memory_order_release);
			return;
		}

		//	呼び出しスレッドがこのノードを抱えている間は、残りの対照ノードをワーカーが引くしかない。
		const auto deadline = std::chrono::steady_clock::now() + kControlWaitLimit;
		while ((g_control_ran_off_caller.load(std::memory_order_acquire) == false) &&
			(std::chrono::steady_clock::now() < deadline))
		{
			std::this_thread::yield();
		}
	}

	//	=================================================================================
	//	ComponentData
	//	全ノードが別々の ComponentData だけを書くので、互いに衝突せず1レイヤーに載る。
	//	=================================================================================

	struct MtMainA : nox::IComponentData { nox::float32 value; };
	struct MtMainB : nox::IComponentData { nox::float32 value; };
	struct MtWork0 : nox::IComponentData { nox::float32 value; };
	struct MtWork1 : nox::IComponentData { nox::float32 value; };
	struct MtWork2 : nox::IComponentData { nox::float32 value; };
	struct MtWork3 : nox::IComponentData { nox::float32 value; };
	struct MtLogicC : nox::IComponentData { nox::float32 value; };

	//	=================================================================================
	//	EntitySystem
	//	=================================================================================

	/// @brief 呼び出しスレッド限定。走ったスレッドを覚える。
	class MtMainOnlyASystem final : public nox::EntitySystem<nox::test::main_thread::MtMainOnlyASystem>
	{
	public:
		static constexpr bool kMainThreadOnly = true;

		void OnUpdate(nox::test::main_thread::MtMainA& a)
		{
			a.value += 1.0f;
			last_thread_id = std::this_thread::get_id();
			on_caller_every_time = on_caller_every_time && (last_thread_id == g_caller_thread_id);
			++call_count;
		}

		std::thread::id last_thread_id{};
		bool on_caller_every_time = true;
		nox::uint32 call_count = 0u;
	};

	/// @brief 呼び出しスレッド限定をもう1つ。1レイヤーに複数あっても全て呼び出しスレッドで走る。
	class MtMainOnlyBSystem final : public nox::EntitySystem<nox::test::main_thread::MtMainOnlyBSystem>
	{
	public:
		static constexpr bool kMainThreadOnly = true;

		void OnUpdate(nox::test::main_thread::MtMainB& b)
		{
			b.value += 1.0f;
			last_thread_id = std::this_thread::get_id();
			on_caller_every_time = on_caller_every_time && (last_thread_id == g_caller_thread_id);
			++call_count;
		}

		std::thread::id last_thread_id{};
		bool on_caller_every_time = true;
		nox::uint32 call_count = 0u;
	};

	/// @brief 対照。kMainThreadOnly を宣言しないので、ワーカーへ配られる。
	template<class TComponent>
	class MtControlSystem final : public nox::EntitySystem<nox::test::main_thread::MtControlSystem<TComponent>>
	{
	public:
		void OnUpdate(TComponent& component)
		{
			component.value += 1.0f;
			nox::test::main_thread::RunControlNode();
			++call_count;
		}

		nox::uint32 call_count = 0u;
	};

	/// @brief false を明示した場合も宣言なしと同じになる。
	class MtExplicitlyFalseSystem final : public nox::EntitySystem<nox::test::main_thread::MtExplicitlyFalseSystem>
	{
	public:
		static constexpr bool kMainThreadOnly = false;

		void OnUpdate(nox::test::main_thread::MtWork0& w) { w.value += 1.0f; }
	};

	//	=================================================================================
	//	EntityLogic
	//	kMainThreadOnly は型単位の宣言で、全更新メソッドに掛かる。
	//	=================================================================================

	class MtMainOnlyLogic final : public nox::EntityLogic<nox::test::main_thread::MtMainOnlyLogic>
	{
	public:
		static constexpr bool kMainThreadOnly = true;

		//	手書きの記述子はメソッドのアドレスを通常の文脈で取るので public に置く。
		void StepA(nox::test::main_thread::MtLogicC& c) { c.value += 1.0f; }
		void StepB(const nox::test::main_thread::MtLogicC& c) { (void)c; }
	};
}

namespace nox
{
	template<>
	struct EntityLogicMethodTable<nox::test::main_thread::MtMainOnlyLogic> final
	{
		static constexpr std::array<nox::EntityLogicMethodDescriptor, 2> k_methods{
			nox::MakeEntityLogicMethodDescriptor<
				&nox::test::main_thread::MtMainOnlyLogic::StepA, nox::SystemPhaseType::Update>("StepA"),
			nox::MakeEntityLogicMethodDescriptor<
				&nox::test::main_thread::MtMainOnlyLogic::StepB, nox::SystemPhaseType::Update>("StepB"),
		};

		[[nodiscard]] static constexpr std::span<const nox::EntityLogicMethodDescriptor> GetMethods()noexcept
		{
			return std::span<const nox::EntityLogicMethodDescriptor>(k_methods.data(), k_methods.size());
		}
	};
}

namespace
{
	using namespace nox::test::main_thread;

	//	nox::EntityLogicStorage は記述子を参照で保持するので、実体に静的記憶域が要る。
	constexpr nox::EntityLogicTypeDescriptor k_main_only_logic_descriptor =
		nox::MakeEntityLogicTypeDescriptor<MtMainOnlyLogic>();

	/// @brief nox::ExecuteUpdaterLayer から EntitySystem を実行する。World の ExecuteNode の代わり。
	void ExecuteSystemNode(void* const context, const nox::UpdaterNode& node)
	{
		node.system->Execute(*static_cast<nox::World*>(context));
	}

	/// @brief 手組みのノード列を実行したときの記録。
	struct NodeRunRecord final
	{
		std::array<std::thread::id, 16> thread_ids{};
		std::array<std::atomic<nox::uint32>, 16> run_counts{};
	};

	/// @brief 手組みのノード1つを実行する。order_index を記録の添字に使う。
	void RecordNodeRun(void* const context, const nox::UpdaterNode& node)
	{
		auto* const record = static_cast<NodeRunRecord*>(context);
		record->thread_ids[node.order_index] = std::this_thread::get_id();
		record->run_counts[node.order_index].fetch_add(1u, std::memory_order_relaxed);
	}
}

//	=====================================================================================
//	1. 記述子
//	=====================================================================================

///	@brief	kMainThreadOnly の宣言が記述子に載る。宣言なし・false の明示は false。
TEST(UpdaterMainThread, DescriptorCarriesTheDeclaration)
{
	EXPECT_TRUE(nox::k_entity_system_type_descriptor<MtMainOnlyASystem>.main_thread_only);
	EXPECT_TRUE(nox::k_entity_system_type_descriptor<MtMainOnlyBSystem>.main_thread_only);
	EXPECT_FALSE(nox::k_entity_system_type_descriptor<MtControlSystem<MtWork0>>.main_thread_only);
	EXPECT_FALSE(nox::k_entity_system_type_descriptor<MtExplicitlyFalseSystem>.main_thread_only);
	EXPECT_TRUE(k_main_only_logic_descriptor.main_thread_only);

	//	kMainThreadOnly と k_parallel_for_each の併記は nox::MakeEntitySystemTypeDescriptor の
	//	static_assert で弾かれる(コンパイルエラーの検証は gtest ではできないので、ここでは書かない)。
	static_assert(nox::IsMainThreadOnlyUpdaterType<MtMainOnlyASystem>());
	static_assert(nox::IsMainThreadOnlyUpdaterType<MtControlSystem<MtWork0>>() == false);
}

//	=====================================================================================
//	2. グラフ
//	=====================================================================================

///	@brief	UpdaterGraph がノードへ写す。EntityLogic は型の全更新メソッドに掛かる。
///	@details	kMainThreadOnly は依存解析に影響しない。全ノードが衝突しないので1レイヤーに載る。
TEST(UpdaterMainThread, GraphNodesCarryTheDeclaration)
{
	MtMainOnlyASystem main_a;
	MtControlSystem<MtWork0> control_0;
	nox::EntityLogicStorage logic_storage{ k_main_only_logic_descriptor };

	std::vector<nox::EntitySystemBase*> systems{ &main_a, &control_0 };
	std::vector<nox::EntityLogicStorage*> storages{ &logic_storage };

	nox::UpdaterGraph graph;
	const nox::UpdaterGraphBuildResult result = graph.TryRebuild(
		std::span<nox::EntitySystemBase* const>(systems.data(), systems.size()),
		std::span<nox::EntityLogicStorage* const>(storages.data(), storages.size()));
	ASSERT_TRUE(result.IsSuccess());

	const std::span<const nox::UpdaterNode> nodes = graph.GetNodes(nox::SystemPhaseType::Update);
	ASSERT_EQ(nodes.size(), 4u);

	nox::uint32 logic_method_count = 0u;
	for (const nox::UpdaterNode& node : nodes)
	{
		if (node.kind == nox::UpdaterNodeKind::EntityLogicMethod)
		{
			EXPECT_TRUE(node.main_thread_only) << node.method->name;
			++logic_method_count;
		}
		else if (node.system == &main_a)
		{
			EXPECT_TRUE(node.main_thread_only);
		}
		else
		{
			EXPECT_FALSE(node.main_thread_only);
		}
	}
	EXPECT_EQ(logic_method_count, 2u);
}

//	=====================================================================================
//	3. 実行
//	=====================================================================================

///	@brief	手組みのノード列で、全ノードがちょうど1回走り、main_thread_only のノードは必ず呼び出しスレッドで走る。
///	@details	ワーカー0本(直列)・1本・3本のいずれでも成り立つ。
TEST(UpdaterMainThread, EveryNodeRunsOnceAndMainThreadOnlyNodesStayOnTheCaller)
{
	static constexpr nox::uint32 kNodeCount = 8u;

	std::array<nox::UpdaterNode, kNodeCount> nodes{};
	for (nox::uint32 index = 0u; index < kNodeCount; ++index)
	{
		nodes[index].order_index = index;
		//	半分(偶数番)を呼び出しスレッド限定にする。
		nodes[index].main_thread_only = ((index % 2u) == 0u);
	}

	for (const nox::uint32 worker_count : { 0u, 1u, 3u })
	{
		nox::JobSystem job_system;
		job_system.Initialize(worker_count);

		NodeRunRecord record;
		nox::ExecuteUpdaterLayer(
			job_system,
			std::span<const nox::UpdaterNode>(nodes.data(), nodes.size()),
			&RecordNodeRun,
			&record);

		job_system.Finalize();

		const std::thread::id caller = std::this_thread::get_id();
		for (nox::uint32 index = 0u; index < kNodeCount; ++index)
		{
			EXPECT_EQ(record.run_counts[index].load(std::memory_order_relaxed), 1u)
				<< "workers=" << worker_count << " node=" << index;
			if (nodes[index].main_thread_only)
			{
				EXPECT_EQ(record.thread_ids[index], caller)
					<< "workers=" << worker_count << " node=" << index;
			}
		}
	}
}

///	@brief	実物の System を UpdaterGraph で組み、ワーカー2本で回す。
///	@details	kMainThreadOnly の System の OnUpdate は毎回呼び出しスレッドで呼ばれ、
///				対照の System は少なくとも1回は呼び出しスレッド以外で呼ばれる。
TEST(UpdaterMainThread, MainThreadOnlySystemsRunOnTheCallerWhileOthersGoToWorkers)
{
	static constexpr nox::uint32 kFrameCount = 16u;

	nox::World world;
	const nox::EntityId entity = world.CreateEntity();
	world.AddComponent<MtMainA>(entity)->value = 0.0f;
	world.AddComponent<MtMainB>(entity)->value = 0.0f;
	world.AddComponent<MtWork0>(entity)->value = 0.0f;
	world.AddComponent<MtWork1>(entity)->value = 0.0f;
	world.AddComponent<MtWork2>(entity)->value = 0.0f;
	world.AddComponent<MtWork3>(entity)->value = 0.0f;

	MtMainOnlyASystem main_a;
	MtMainOnlyBSystem main_b;
	MtControlSystem<MtWork0> control_0;
	MtControlSystem<MtWork1> control_1;
	MtControlSystem<MtWork2> control_2;
	MtControlSystem<MtWork3> control_3;

	std::vector<nox::EntitySystemBase*> systems{ &main_a, &main_b, &control_0, &control_1, &control_2, &control_3 };
	for (nox::EntitySystemBase* const system : systems)
	{
		world.BuildQuery(system->GetQuery(), system->GetDescriptor().make_read_write_mask());
	}

	nox::UpdaterGraph graph;
	const nox::UpdaterGraphBuildResult result = graph.TryRebuild(
		std::span<nox::EntitySystemBase* const>(systems.data(), systems.size()),
		std::span<nox::EntityLogicStorage* const>());
	ASSERT_TRUE(result.IsSuccess());

	//	前提: 全ノードが1レイヤーに載っていなければ、配る経路は踏まれない。
	ASSERT_EQ(graph.GetLayerCount(nox::SystemPhaseType::Update), 1u);
	ASSERT_EQ(graph.GetLayerNodes(nox::SystemPhaseType::Update, 0u).size(), systems.size());

	g_caller_thread_id = std::this_thread::get_id();
	g_control_ran_off_caller.store(false, std::memory_order_release);

	nox::JobSystem job_system;
	job_system.Initialize(2u);
	ASSERT_EQ(job_system.GetWorkerCount(), 2u);

	for (nox::uint32 frame = 0u; frame < kFrameCount; ++frame)
	{
		nox::ExecuteUpdaterLayer(
			job_system,
			graph.GetLayerNodes(nox::SystemPhaseType::Update, 0u),
			&ExecuteSystemNode,
			&world);
	}

	job_system.Finalize();

	EXPECT_EQ(main_a.call_count, kFrameCount);
	EXPECT_EQ(main_b.call_count, kFrameCount);
	EXPECT_TRUE(main_a.on_caller_every_time);
	EXPECT_TRUE(main_b.on_caller_every_time);
	EXPECT_EQ(main_a.last_thread_id, g_caller_thread_id);
	EXPECT_EQ(main_b.last_thread_id, g_caller_thread_id);

	EXPECT_EQ(control_0.call_count, kFrameCount);
	EXPECT_EQ(control_1.call_count, kFrameCount);
	EXPECT_EQ(control_2.call_count, kFrameCount);
	EXPECT_EQ(control_3.call_count, kFrameCount);
	EXPECT_TRUE(g_control_ran_off_caller.load(std::memory_order_acquire))
		<< "kMainThreadOnly の無い System が一度も呼び出しスレッド以外で走っていません(ワーカーへ配られていない)";
}
