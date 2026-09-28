//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	updater_service_node_test.cpp
///	@brief	Serviceの属性付きメソッドと Task(属性付きのグローバル関数)が UpdaterGraph のノードになることの検証。
///	@details	どちらも「1フェーズに1回だけ呼ばれ、entityを列挙しない」ノード。ここで固定するのは次のとおり。
///
///				  ・記述子 … Serviceのメソッドは自分自身への書き込みを暗黙に宣言に含める。
///				             実行スレッド(nox::attr::ThreadAffinity)と EntityCommands の有無が記述子に載る
///				  ・グラフ … 同じServiceのメソッド同士はメソッド名順に直列化される(group の衝突)。
///				             そのServiceを const で読む System は、自己書き込みとの衝突で後ろのレイヤーへ回る。
///				             Service の型に対する RunAfter が効く。Service を書く Task と読む System は直列化される
///				  ・実行   … entityが0個でも、フェーズごとにちょうど1回呼ばれる。MainThread のメソッドは呼び出しスレッドで走る。
///				             EntityCommands& で積んだ生成はフェーズ末の反映で見えるようになる
///				  ・生成器 … ヘッダに定義した Service(private メソッド)と Task が生成コードの表に載り、呼べる
///				  ・排他   … nox::World& を取るメソッド / Task は全ノードと衝突して単独のレイヤーになり、
///				             属性が Any でもメインスレッド限定で、フェーズごとに1回 World を受け取って呼ばれる
///
///	@note		World::Init は private なので、フェーズは World と同じ部品(nox::UpdaterGraph /
///				nox::ExecuteUpdaterLayer / nox::WorldNodeCommandScope / FlushEntityCommands)を並べて回す
///				(updater_main_thread_test.cpp / entity_command_playback_test.cpp と同じ考え)。
///				遅延系は「フェーズ実行中または列挙中」にしか積めないので、フェーズの代わりに列挙スコープで囲む。
///
///	@note		手書きの記述子(nox::ServiceMethodTable の特殊化)を使う型はこの翻訳単位にだけ置く。
///				生成器の経路は entity_ecs_test.h の nox::test::ecs::TestNodeService / TestNodeCountTask で見る。

#include	"pch.h"

//	core_test の pch.h は gtest しか載せていない (test_new_delete.cpp の都合)。
//	core のヘッダは kernel / reflection の基盤型に依存するので、main.cpp と同じ順で先に入れる。
#include	"../../kernel/kernel.h"
#include	"../../kernel/job_system.h"
#include	"../../reflection/reflection.h"

#include	"../world.h"
#include	"../updater_graph.h"
#include	"../service_method.h"
#include	"../updater_task.h"
#include	"../entity_type_registry.h"
#include	"entity_ecs_test.h"

#include	<atomic>
#include	<thread>
#include	<vector>

namespace nox::test::service_node
{
	struct SnHealth : nox::IComponentData { nox::int32 value; };
	struct SnMarker : nox::IComponentData { nox::int32 value; };
	struct SnOther : nox::IComponentData { nox::int32 value; };

	/// @brief 呼び出しスレッド(= フェーズを回しているスレッド)の ID。テストの先頭で入れる。
	inline std::thread::id g_caller_thread_id{};

	/// @brief SnIdleTask が呼ばれた回数。ワーカー上で呼ばれうるので atomic にする。
	inline std::atomic<nox::int32> g_idle_task_calls{ 0 };

	/// @brief 3つのメソッドを持つService。記述子は手書きの表で作る。
	/// @details 型名順では SnAReadAfterSystem("SnAR") の後ろ、SnBravoReadSystem("SnB") の前。
	class SnAlphaService final : public nox::Service
	{
	public:
		//	手書きの記述子はメソッドのアドレスを通常の文脈で取るので public に置く。
		//	(生成器の経路は private のままでよい。nox::test::ecs::TestNodeService を参照)

		/// @brief 引数なし。メインスレッド限定で宣言する。
		void Poll()
		{
			++poll_count;
			poll_on_caller_every_time = poll_on_caller_every_time && (std::this_thread::get_id() == g_caller_thread_id);
		}

		/// @brief 遅延生成を積む。
		void Spawn(nox::EntityCommands& commands)
		{
			last_spawned = commands.Create();
			commands.Add<nox::test::service_node::SnMarker>(last_spawned, nox::test::service_node::SnMarker{ .value = 7 });
			++spawn_count;
		}

		void Tick()
		{
			++tick_count;
		}

		nox::int32 poll_count = 0;
		nox::int32 spawn_count = 0;
		nox::int32 tick_count = 0;
		/// @brief SnWriteTask が書き込んだ回数。
		nox::int32 task_writes = 0;
		bool poll_on_caller_every_time = true;
		nox::EntityId last_spawned{};
	};

	/// @brief SnAlphaService を const で読む System。自己書き込みとの衝突で全メソッドの後ろへ回る。
	class SnBravoReadSystem final : public nox::EntitySystem<nox::test::service_node::SnBravoReadSystem>
	{
	public:
		void OnUpdate(nox::test::service_node::SnHealth& health, const nox::test::service_node::SnAlphaService& service)
		{
			health.value = service.tick_count;
		}
	};

	/// @brief 型名順では SnAlphaService より前だが、RunAfter でその全メソッドの後ろへ回る。Serviceには触れない。
	class SnAReadAfterSystem final : public nox::EntitySystem<nox::test::service_node::SnAReadAfterSystem>
	{
	public:
		using RunAfter = nox::TypeList<nox::test::service_node::SnAlphaService>;

		void OnUpdate(nox::test::service_node::SnOther& other)
		{
			++other.value;
		}
	};

	/// @brief SnAlphaService へ書き込む Task。型名順では SnBravoReadSystem の後ろ。
	void SnWriteTask(nox::test::service_node::SnAlphaService& service)
	{
		++service.task_writes;
	}

	/// @brief 何も宣言しない Task。どのノードとも衝突しないので先頭レイヤーに載る。
	void SnIdleTask()
	{
		g_idle_task_calls.fetch_add(1, std::memory_order_relaxed);
	}
}

namespace nox
{
	template<>
	struct ServiceMethodTable<nox::test::service_node::SnAlphaService> final
	{
		static constexpr std::array<nox::ServiceMethodDescriptor, 3> k_methods{
			nox::MakeServiceMethodDescriptor<
				&nox::test::service_node::SnAlphaService::Poll,
				nox::SystemPhaseType::Update,
				nox::attr::ThreadAffinity::MainThread>("Poll"),
			nox::MakeServiceMethodDescriptor<
				&nox::test::service_node::SnAlphaService::Spawn, nox::SystemPhaseType::Update>("Spawn"),
			nox::MakeServiceMethodDescriptor<
				&nox::test::service_node::SnAlphaService::Tick, nox::SystemPhaseType::Update>("Tick"),
		};

		[[nodiscard]] static constexpr std::span<const nox::ServiceMethodDescriptor> GetMethods()noexcept
		{
			return std::span<const nox::ServiceMethodDescriptor>(k_methods.data(), k_methods.size());
		}
	};
}

namespace
{
	using namespace nox::test::service_node;

	//	UpdaterGraph のノードは記述子を指すので、実体に静的記憶域が要る。
	constexpr nox::ServiceMethodTypeDescriptor k_alpha_descriptor = nox::MakeServiceMethodTypeDescriptor<SnAlphaService>();

	constexpr nox::UpdaterTaskDescriptor k_write_task =
		nox::MakeUpdaterTaskDescriptor<&nox::test::service_node::SnWriteTask, nox::SystemPhaseType::Update>(
			"nox::test::service_node::SnWriteTask");

	constexpr nox::UpdaterTaskDescriptor k_idle_task =
		nox::MakeUpdaterTaskDescriptor<&nox::test::service_node::SnIdleTask, nox::SystemPhaseType::Update>(
			"nox::test::service_node::SnIdleTask");

	//	引数の規則(1フェーズに1回のノード)。ComponentData / EntityId を弾く側は static_assert なので書けない。
	static_assert(nox::detail::ValidateOncePerFrameSignature<nox::EntitySignature<>>());
	static_assert(nox::detail::ValidateOncePerFrameSignature<
		nox::EntitySignature<SnAlphaService&, const SnAlphaService*, nox::EntityCommands&>>());

	/// @brief ノード1つを実行する。World の ExecuteNode の代わり(EntityLogic はここでは使わない)。
	void ExecuteNodeForTest(void* const context, const nox::UpdaterNode& node)
	{
		nox::World& world = *static_cast<nox::World*>(context);
		const nox::WorldNodeCommandScope command_scope(world, node.command_buffer_index);
		switch (node.kind)
		{
		case nox::UpdaterNodeKind::EntitySystem:
			node.system->Execute(world);
			break;
		case nox::UpdaterNodeKind::ServiceMethod:
			node.service_method->invoke(*node.service, world);
			break;
		case nox::UpdaterNodeKind::Task:
			node.task->invoke(world);
			break;
		case nox::UpdaterNodeKind::EntityLogicMethod:
			break;
		}
	}

	/// @brief Update フェーズを1回回す。World::ExecutePhase と同じ順(レイヤー順 → 反映)。
	/// @details 反映の直前に呼ぶ関数を渡せる(反映前の状態を見るため)。
	template<class BeforeFlush>
	void RunUpdatePhase(nox::World& world, const nox::UpdaterGraph& graph, nox::JobSystem& job_system, BeforeFlush&& before_flush)
	{
		world.EnterEntityIteration();
		const nox::uint32 layer_count = graph.GetLayerCount(nox::SystemPhaseType::Update);
		for (nox::uint32 layer_index = 0u; layer_index < layer_count; ++layer_index)
		{
			nox::ExecuteUpdaterLayer(
				job_system,
				graph.GetLayerNodes(nox::SystemPhaseType::Update, layer_index),
				&ExecuteNodeForTest,
				&world);
		}
		world.LeaveEntityIteration();

		before_flush();
		world.FlushEntityCommands();
	}

	[[nodiscard]] const nox::UpdaterNode* FindNode(
		const std::span<const nox::UpdaterNode> nodes,
		const std::string_view type_name,
		const std::string_view method_name = std::string_view())noexcept
	{
		for (const nox::UpdaterNode& node : nodes)
		{
			if ((nox::GetUpdaterNodeTypeName(node) == type_name) && (nox::GetUpdaterNodeMethodName(node) == method_name))
			{
				return &node;
			}
		}
		return nullptr;
	}

	[[nodiscard]] const nox::ServiceMethodDescriptor* FindMethod(
		const std::span<const nox::ServiceMethodDescriptor> methods,
		const std::string_view name)noexcept
	{
		for (const nox::ServiceMethodDescriptor& method : methods)
		{
			if (method.name == name)
			{
				return &method;
			}
		}
		return nullptr;
	}

	[[nodiscard]] const nox::ServiceMethodTypeDescriptor* FindGeneratedServiceType(const std::string_view name)noexcept
	{
		for (const nox::ServiceMethodTypeDescriptor* const descriptor : nox::GetServiceMethodTypes())
		{
			if (descriptor->name == name)
			{
				return descriptor;
			}
		}
		return nullptr;
	}

	[[nodiscard]] const nox::UpdaterTaskDescriptor* FindGeneratedTask(const std::string_view name)noexcept
	{
		for (const nox::UpdaterTaskDescriptor* const task : nox::GetUpdaterTaskDescriptors())
		{
			if (task->name == name)
			{
				return task;
			}
		}
		return nullptr;
	}
}

//	=====================================================================================
//	1. 記述子
//	=====================================================================================

///	@brief	Serviceのメソッドは自分自身への書き込みを宣言の末尾に含む。実行スレッドと EntityCommands の有無が載る。
TEST(ServiceNode, DescriptorDeclaresSelfWriteAndAffinity)
{
	EXPECT_EQ(k_alpha_descriptor.name, nox::util::GetTypeName<SnAlphaService>());
	EXPECT_EQ(k_alpha_descriptor.type, &nox::reflection::Typeof<SnAlphaService>());

	const std::span<const nox::ServiceMethodDescriptor> methods = k_alpha_descriptor.get_methods();
	ASSERT_EQ(methods.size(), 3u);
	for (const nox::ServiceMethodDescriptor& method : methods)
	{
		const std::span<const nox::ServiceAccess> accesses = method.get_service_accesses();
		//	どのメソッドも引数に Service を取らないので、宣言は自己書き込みの1つだけ。
		ASSERT_EQ(accesses.size(), 1u) << method.name;
		EXPECT_EQ(accesses[0].type, &nox::reflection::Typeof<SnAlphaService>()) << method.name;
		EXPECT_TRUE(accesses[0].write) << method.name;
		EXPECT_EQ(method.phase, nox::SystemPhaseType::Update) << method.name;
	}

	EXPECT_TRUE(methods[0].main_thread_only);
	EXPECT_FALSE(methods[1].main_thread_only);
	EXPECT_FALSE(methods[2].main_thread_only);
	EXPECT_FALSE(methods[0].emits_structural_change);
	EXPECT_TRUE(methods[1].emits_structural_change);
	EXPECT_FALSE(methods[2].emits_structural_change);

	//	Task は引数の宣言だけ(自分のインスタンスを持たない)。
	ASSERT_EQ(k_write_task.get_service_accesses().size(), 1u);
	EXPECT_EQ(k_write_task.get_service_accesses()[0].type, &nox::reflection::Typeof<SnAlphaService>());
	EXPECT_TRUE(k_write_task.get_service_accesses()[0].write);
	EXPECT_TRUE(k_idle_task.get_service_accesses().empty());
	EXPECT_FALSE(k_idle_task.main_thread_only);
	EXPECT_FALSE(k_idle_task.emits_structural_change);
}

//	=====================================================================================
//	2. グラフ
//	=====================================================================================

///	@brief	同じServiceのメソッド同士は、宣言が重ならなくてもメソッド名順に直列化される。
TEST(ServiceNode, MethodsOfTheSameServiceAreSerializedInMethodNameOrder)
{
	SnAlphaService service;
	const std::array<nox::UpdaterServiceBinding, 1> bindings{
		nox::UpdaterServiceBinding{ .service = &service, .descriptor = &k_alpha_descriptor },
	};

	nox::UpdaterGraph graph;
	const nox::UpdaterGraphBuildResult result = graph.TryRebuild(
		std::span<nox::EntitySystemBase* const>(),
		std::span<nox::EntityLogicStorage* const>(),
		std::span<const nox::UpdaterServiceBinding>(bindings.data(), bindings.size()),
		std::span<const nox::UpdaterTaskDescriptor* const>());
	ASSERT_TRUE(result.IsSuccess());

	const std::span<const nox::UpdaterNode> nodes = graph.GetNodes(nox::SystemPhaseType::Update);
	ASSERT_EQ(nodes.size(), 3u);
	EXPECT_EQ(graph.GetLayerCount(nox::SystemPhaseType::Update), 3u);

	const std::array<std::string_view, 3> expected_methods{ "Poll", "Spawn", "Tick" };
	for (const nox::UpdaterNode& node : nodes)
	{
		ASSERT_EQ(node.kind, nox::UpdaterNodeKind::ServiceMethod);
		EXPECT_EQ(node.service, &service);
		EXPECT_EQ(node.service_type, &k_alpha_descriptor);
		ASSERT_LT(node.order_index, expected_methods.size());
		EXPECT_EQ(nox::GetUpdaterNodeMethodName(node), expected_methods[node.order_index]);
		EXPECT_EQ(nox::GetUpdaterNodeTypeName(node), nox::util::GetTypeName<SnAlphaService>());
		//	鎖なので全順序の位置がそのままレイヤー番号になる。
		EXPECT_EQ(node.layer_index, node.order_index);
		EXPECT_EQ(node.main_thread_only, node.order_index == 0u);
		EXPECT_NE(node.access.group_index, nox::k_invalid_updater_group_index);
	}

	for (size_t i = 0u; i < nodes.size(); ++i)
	{
		for (size_t j = i + 1u; j < nodes.size(); ++j)
		{
			EXPECT_TRUE(nox::ConflictsUpdaterNodeAccess(nodes[i].access, nodes[j].access));
		}
	}

	//	コマンドバッファは EntityCommands& を取る Spawn の1本だけ。
	EXPECT_EQ(graph.GetCommandBufferCount(nox::SystemPhaseType::Update), 1u);
	const nox::UpdaterNode* const spawn = FindNode(nodes, nox::util::GetTypeName<SnAlphaService>(), "Spawn");
	ASSERT_NE(spawn, nullptr);
	EXPECT_EQ(spawn->command_buffer_index, 0u);
}

///	@brief	Serviceを const で読む System は自己書き込みと衝突して後ろへ回る。Service の型への RunAfter も効く。
TEST(ServiceNode, ReadersAndRunAfterOfTheServiceComeAfterItsMethods)
{
	SnAlphaService service;
	SnBravoReadSystem reader;
	SnAReadAfterSystem after;
	std::vector<nox::EntitySystemBase*> systems{ &reader, &after };
	const std::array<nox::UpdaterServiceBinding, 1> bindings{
		nox::UpdaterServiceBinding{ .service = &service, .descriptor = &k_alpha_descriptor },
	};

	nox::UpdaterGraph graph;
	const nox::UpdaterGraphBuildResult result = graph.TryRebuild(
		std::span<nox::EntitySystemBase* const>(systems.data(), systems.size()),
		std::span<nox::EntityLogicStorage* const>(),
		std::span<const nox::UpdaterServiceBinding>(bindings.data(), bindings.size()),
		std::span<const nox::UpdaterTaskDescriptor* const>());
	ASSERT_TRUE(result.IsSuccess());

	const std::span<const nox::UpdaterNode> nodes = graph.GetNodes(nox::SystemPhaseType::Update);
	ASSERT_EQ(nodes.size(), 5u);

	const std::string_view service_name = nox::util::GetTypeName<SnAlphaService>();
	const nox::UpdaterNode* const tick = FindNode(nodes, service_name, "Tick");
	const nox::UpdaterNode* const reader_node = FindNode(nodes, nox::util::GetTypeName<SnBravoReadSystem>());
	const nox::UpdaterNode* const after_node = FindNode(nodes, nox::util::GetTypeName<SnAReadAfterSystem>());
	ASSERT_NE(tick, nullptr);
	ASSERT_NE(reader_node, nullptr);
	ASSERT_NE(after_node, nullptr);

	//	読む System は宣言の衝突(自己書き込み vs const 読み)だけで後ろへ回る。
	EXPECT_TRUE(nox::ConflictsUpdaterNodeAccess(tick->access, reader_node->access));
	EXPECT_GT(reader_node->layer_index, tick->layer_index);
	EXPECT_GT(reader_node->order_index, tick->order_index);

	//	RunAfter の System は衝突しないが、明示辺で全メソッドの後ろへ回る(型名順なら先頭)。
	EXPECT_FALSE(nox::ConflictsUpdaterNodeAccess(tick->access, after_node->access));
	EXPECT_GT(after_node->layer_index, tick->layer_index);
	EXPECT_GT(after_node->order_index, tick->order_index);
	EXPECT_LT(nox::util::GetTypeName<SnAReadAfterSystem>(), service_name);
}

///	@brief	Service を書く Task と読む System は直列化される。何も宣言しない Task は先頭レイヤーに載る。
TEST(ServiceNode, TaskWritingAServiceIsSerializedWithItsReaders)
{
	SnBravoReadSystem reader;
	std::vector<nox::EntitySystemBase*> systems{ &reader };
	const std::array<const nox::UpdaterTaskDescriptor*, 2> tasks{ &k_write_task, &k_idle_task };

	nox::UpdaterGraph graph;
	const nox::UpdaterGraphBuildResult result = graph.TryRebuild(
		std::span<nox::EntitySystemBase* const>(systems.data(), systems.size()),
		std::span<nox::EntityLogicStorage* const>(),
		std::span<const nox::UpdaterServiceBinding>(),
		std::span<const nox::UpdaterTaskDescriptor* const>(tasks.data(), tasks.size()));
	ASSERT_TRUE(result.IsSuccess());

	const std::span<const nox::UpdaterNode> nodes = graph.GetNodes(nox::SystemPhaseType::Update);
	ASSERT_EQ(nodes.size(), 3u);

	const nox::UpdaterNode* const write_node = FindNode(nodes, k_write_task.name);
	const nox::UpdaterNode* const idle_node = FindNode(nodes, k_idle_task.name);
	const nox::UpdaterNode* const reader_node = FindNode(nodes, nox::util::GetTypeName<SnBravoReadSystem>());
	ASSERT_NE(write_node, nullptr);
	ASSERT_NE(idle_node, nullptr);
	ASSERT_NE(reader_node, nullptr);

	EXPECT_EQ(write_node->kind, nox::UpdaterNodeKind::Task);
	EXPECT_EQ(write_node->task, &k_write_task);
	EXPECT_EQ(write_node->access.group_index, nox::k_invalid_updater_group_index);
	EXPECT_TRUE(nox::GetUpdaterNodeMethodName(*write_node).empty());

	EXPECT_TRUE(nox::ConflictsUpdaterNodeAccess(write_node->access, reader_node->access));
	EXPECT_NE(write_node->layer_index, reader_node->layer_index);
	//	全順序は型名 / 関数名の昇順("SnBravoReadSystem" < "SnWriteTask")。
	EXPECT_LT(reader_node->order_index, write_node->order_index);
	EXPECT_EQ(idle_node->layer_index, 0u);
}

///	@brief	RunAfter に並べた Service が(属性付きメソッドを持って)登録されていなければ構築失敗になる。
TEST(ServiceNode, RunAfterAnUnboundServiceFails)
{
	SnAReadAfterSystem after;
	std::vector<nox::EntitySystemBase*> systems{ &after };

	nox::UpdaterGraph graph;
	const nox::UpdaterGraphBuildResult result = graph.TryRebuild(
		std::span<nox::EntitySystemBase* const>(systems.data(), systems.size()),
		std::span<nox::EntityLogicStorage* const>());
	EXPECT_EQ(result.error, nox::UpdaterGraphBuildError::UnresolvedOrderTarget);
	EXPECT_EQ(result.target_type_name, nox::util::GetTypeName<SnAlphaService>());
	EXPECT_TRUE(graph.GetNodes(nox::SystemPhaseType::Update).empty());
}

//	=====================================================================================
//	3. 実行
//	=====================================================================================

///	@brief	entityが0個でも、Serviceの各メソッドと Task はフェーズごとにちょうど1回呼ばれる。
///	@details	ワーカー2本で回す。Poll(MainThread)と SnIdleTask は同じ先頭レイヤーに載るので配る経路を通り、
///				Poll は毎回呼び出しスレッドで走る。Spawn が積んだ生成はフェーズ末の反映まで見えない。
TEST(ServiceNode, EachMethodAndTaskRunsExactlyOncePerPhase)
{
	static constexpr nox::int32 kFrameCount = 8;

	nox::World world;
	world.RegisterService(*new SnAlphaService());
	ASSERT_TRUE(world.TryInitializeServices().IsSuccess());
	SnAlphaService* const service = world.TryGetService<SnAlphaService>();
	ASSERT_NE(service, nullptr);

	SnBravoReadSystem reader;
	world.BuildQuery(reader.GetQuery(), reader.GetDescriptor().make_read_write_mask());
	std::vector<nox::EntitySystemBase*> systems{ &reader };
	const std::array<nox::UpdaterServiceBinding, 1> bindings{
		nox::UpdaterServiceBinding{ .service = service, .descriptor = &k_alpha_descriptor },
	};
	const std::array<const nox::UpdaterTaskDescriptor*, 2> tasks{ &k_write_task, &k_idle_task };

	nox::UpdaterGraph graph;
	const nox::UpdaterGraphBuildResult result = graph.TryRebuild(
		std::span<nox::EntitySystemBase* const>(systems.data(), systems.size()),
		std::span<nox::EntityLogicStorage* const>(),
		std::span<const nox::UpdaterServiceBinding>(bindings.data(), bindings.size()),
		std::span<const nox::UpdaterTaskDescriptor* const>(tasks.data(), tasks.size()));
	ASSERT_TRUE(result.IsSuccess());

	//	前提: Poll と SnIdleTask が同じレイヤーに載っていなければ、配る経路は踏まれない。
	const std::span<const nox::UpdaterNode> nodes = graph.GetNodes(nox::SystemPhaseType::Update);
	const nox::UpdaterNode* const poll = FindNode(nodes, nox::util::GetTypeName<SnAlphaService>(), "Poll");
	const nox::UpdaterNode* const idle = FindNode(nodes, k_idle_task.name);
	ASSERT_NE(poll, nullptr);
	ASSERT_NE(idle, nullptr);
	ASSERT_EQ(poll->layer_index, idle->layer_index);
	ASSERT_TRUE(poll->main_thread_only);

	world.ReserveNodeEntityCommandBuffers(graph.GetCommandBufferCount(nox::SystemPhaseType::Update));

	g_caller_thread_id = std::this_thread::get_id();
	g_idle_task_calls.store(0, std::memory_order_relaxed);

	nox::JobSystem job_system;
	job_system.Initialize(2u);

	for (nox::int32 frame = 0; frame < kFrameCount; ++frame)
	{
		RunUpdatePhase(world, graph, job_system, [&world, service]()
			{
				//	遅延生成は反映前には見えない(Idは払い出し済み)。
				EXPECT_TRUE(world.IsAlive(service->last_spawned));
				EXPECT_FALSE(world.HasComponent<SnMarker>(service->last_spawned));
			});

		//	反映後は Spawn が積んだ ComponentData が見える。
		ASSERT_TRUE(world.HasComponent<SnMarker>(service->last_spawned));
		EXPECT_EQ(world.TryGetComponent<SnMarker>(service->last_spawned)->value, 7);
	}

	job_system.Finalize();

	EXPECT_EQ(service->poll_count, kFrameCount);
	EXPECT_EQ(service->spawn_count, kFrameCount);
	EXPECT_EQ(service->tick_count, kFrameCount);
	EXPECT_EQ(service->task_writes, kFrameCount);
	EXPECT_EQ(g_idle_task_calls.load(std::memory_order_relaxed), kFrameCount);
	EXPECT_TRUE(service->poll_on_caller_every_time);
}

//	=====================================================================================
//	4. 生成器の経路
//	=====================================================================================

///	@brief	ヘッダに定義した Service の属性付き private メソッドと Task が、生成コードの表に載る。
TEST(ServiceNode, GeneratedTablesCarryHeaderDefinedServiceAndTask)
{
	using nox::test::ecs::TestCounterService;
	using nox::test::ecs::TestNodeService;

	const nox::ServiceMethodTypeDescriptor* const descriptor =
		FindGeneratedServiceType(nox::util::GetTypeName<TestNodeService>());
	ASSERT_NE(descriptor, nullptr) << "ヘッダに定義した Service の属性付きメソッドが生成コードの表に載っていません";
	EXPECT_EQ(descriptor->type, &nox::reflection::Typeof<TestNodeService>());
	ASSERT_EQ(descriptor->get_methods().size(), 3u);

	const nox::ServiceMethodDescriptor* const poll = FindMethod(descriptor->get_methods(), "Poll");
	const nox::ServiceMethodDescriptor* const spawn = FindMethod(descriptor->get_methods(), "Spawn");
	const nox::ServiceMethodDescriptor* const observe = FindMethod(descriptor->get_methods(), "Observe");
	ASSERT_NE(poll, nullptr);
	ASSERT_NE(spawn, nullptr);
	ASSERT_NE(observe, nullptr);

	//	属性の第2引数(ThreadAffinity)はメソッド単位。
	EXPECT_TRUE(poll->main_thread_only);
	EXPECT_FALSE(spawn->main_thread_only);
	EXPECT_FALSE(observe->main_thread_only);
	EXPECT_TRUE(spawn->emits_structural_change);
	EXPECT_FALSE(observe->emits_structural_change);

	//	Observe は TestCounterService を const で読み、末尾に自己書き込みが足される。
	const std::span<const nox::ServiceAccess> accesses = observe->get_service_accesses();
	ASSERT_EQ(accesses.size(), 2u);
	EXPECT_EQ(accesses[0].type, &nox::reflection::Typeof<TestCounterService>());
	EXPECT_FALSE(accesses[0].write);
	EXPECT_EQ(accesses[1].type, &nox::reflection::Typeof<TestNodeService>());
	EXPECT_TRUE(accesses[1].write);

	const nox::UpdaterTaskDescriptor* const task = FindGeneratedTask("nox::test::ecs::TestNodeCountTask");
	ASSERT_NE(task, nullptr) << "ヘッダに定義した Task が生成コードの表に載っていません";
	EXPECT_EQ(task->phase, nox::SystemPhaseType::Update);
	EXPECT_FALSE(task->main_thread_only);
	ASSERT_EQ(task->get_service_accesses().size(), 1u);
	EXPECT_EQ(task->get_service_accesses()[0].type, &nox::reflection::Typeof<TestNodeService>());
	EXPECT_TRUE(task->get_service_accesses()[0].write);
}

///	@brief	生成コードの記述子で組んだグラフを1フェーズ回すと、private メソッドと Task が1回ずつ呼ばれる。
TEST(ServiceNode, GeneratedDescriptorsRunPrivateMethodsAndTaskOncePerPhase)
{
	using nox::test::ecs::TestCounterService;
	using nox::test::ecs::TestHealth;
	using nox::test::ecs::TestNodeService;

	const nox::ServiceMethodTypeDescriptor* const descriptor =
		FindGeneratedServiceType(nox::util::GetTypeName<TestNodeService>());
	const nox::UpdaterTaskDescriptor* const task = FindGeneratedTask("nox::test::ecs::TestNodeCountTask");
	ASSERT_NE(descriptor, nullptr);
	ASSERT_NE(task, nullptr);

	nox::World world;
	world.RegisterService(*new TestNodeService());
	world.RegisterService(*new TestCounterService());
	ASSERT_TRUE(world.TryInitializeServices().IsSuccess());
	TestNodeService* const node_service = world.TryGetService<TestNodeService>();
	TestCounterService* const counter = world.TryGetService<TestCounterService>();
	ASSERT_NE(node_service, nullptr);
	ASSERT_NE(counter, nullptr);
	counter->call_count = 3;

	const std::array<nox::UpdaterServiceBinding, 1> bindings{
		nox::UpdaterServiceBinding{ .service = node_service, .descriptor = descriptor },
	};
	const std::array<const nox::UpdaterTaskDescriptor*, 1> tasks{ task };

	nox::UpdaterGraph graph;
	const nox::UpdaterGraphBuildResult result = graph.TryRebuild(
		std::span<nox::EntitySystemBase* const>(),
		std::span<nox::EntityLogicStorage* const>(),
		std::span<const nox::UpdaterServiceBinding>(bindings.data(), bindings.size()),
		std::span<const nox::UpdaterTaskDescriptor* const>(tasks.data(), tasks.size()));
	ASSERT_TRUE(result.IsSuccess());

	//	Task は TestNodeService へ書くので、同じServiceのメソッドと全て直列化される(4ノードが4レイヤー)。
	EXPECT_EQ(graph.GetNodes(nox::SystemPhaseType::Update).size(), 4u);
	EXPECT_EQ(graph.GetLayerCount(nox::SystemPhaseType::Update), 4u);

	world.ReserveNodeEntityCommandBuffers(graph.GetCommandBufferCount(nox::SystemPhaseType::Update));

	g_caller_thread_id = std::this_thread::get_id();
	nox::JobSystem job_system;
	job_system.Initialize(0u);
	RunUpdatePhase(world, graph, job_system, []() {});
	job_system.Finalize();

	EXPECT_EQ(node_service->poll_count, 1);
	EXPECT_EQ(node_service->spawn_count, 1);
	EXPECT_EQ(node_service->observe_count, 1);
	EXPECT_EQ(node_service->observed_counter, 3);
	EXPECT_EQ(node_service->task_count, 1);
	ASSERT_TRUE(world.HasComponent<TestHealth>(node_service->last_spawned));
	EXPECT_EQ(world.TryGetComponent<TestHealth>(node_service->last_spawned)->value, 5);
}

//	=====================================================================================
//	5. 排他アクセス(nox::World& を引数に取るノード)
//	=====================================================================================

namespace nox::test::service_node
{
	/// @brief SnExclusiveTask が呼ばれた回数と、毎回呼び出しスレッドで走ったか。
	inline std::atomic<nox::int32> g_exclusive_task_calls{ 0 };
	inline std::atomic<bool> g_exclusive_task_on_caller_every_time{ true };
	/// @brief SnOtherIdleTask が呼ばれた回数。ワーカー上で呼ばれうるので atomic にする。
	inline std::atomic<nox::int32> g_other_idle_task_calls{ 0 };

	/// @brief nox::World& を取るメソッドを持つService。属性の実行スレッドは Any のまま(排他で暗黙にメインスレッド限定になる)。
	class SnExclusiveService final : public nox::Service
	{
	public:
		void Ingress(nox::World& world)
		{
			++ingress_count;
			last_world = &world;
			ingress_on_caller_every_time = ingress_on_caller_every_time && (std::this_thread::get_id() == g_caller_thread_id);
		}

		nox::int32 ingress_count = 0;
		nox::World* last_world = nullptr;
		bool ingress_on_caller_every_time = true;
	};

	/// @brief nox::World& を取る Task。
	void SnExclusiveTask([[maybe_unused]] nox::World& world)
	{
		g_exclusive_task_calls.fetch_add(1, std::memory_order_relaxed);
		if (std::this_thread::get_id() != g_caller_thread_id)
		{
			g_exclusive_task_on_caller_every_time.store(false, std::memory_order_relaxed);
		}
	}

	/// @brief 何も宣言しない Task(SnIdleTask と別の関数)。
	void SnOtherIdleTask()
	{
		g_other_idle_task_calls.fetch_add(1, std::memory_order_relaxed);
	}
}

namespace nox
{
	template<>
	struct ServiceMethodTable<nox::test::service_node::SnExclusiveService> final
	{
		static constexpr std::array<nox::ServiceMethodDescriptor, 1> k_methods{
			nox::MakeServiceMethodDescriptor<
				&nox::test::service_node::SnExclusiveService::Ingress, nox::SystemPhaseType::FrameIngress>("Ingress"),
		};

		[[nodiscard]] static constexpr std::span<const nox::ServiceMethodDescriptor> GetMethods()noexcept
		{
			return std::span<const nox::ServiceMethodDescriptor>(k_methods.data(), k_methods.size());
		}
	};
}

namespace
{
	using namespace nox::test::service_node;

	constexpr nox::ServiceMethodTypeDescriptor k_exclusive_descriptor = nox::MakeServiceMethodTypeDescriptor<SnExclusiveService>();

	constexpr nox::UpdaterTaskDescriptor k_exclusive_task =
		nox::MakeUpdaterTaskDescriptor<&nox::test::service_node::SnExclusiveTask, nox::SystemPhaseType::FrameIngress>(
			"nox::test::service_node::SnExclusiveTask");

	//	全順序のキーは記述子に渡した名前なので、同じ関数でも名前で前後を決められる。
	//	名前順: SnAIdle < SnBIdle < SnExclusiveService < SnExclusiveTask < SnYIdle < SnZIdle
	constexpr nox::UpdaterTaskDescriptor k_ingress_idle_a =
		nox::MakeUpdaterTaskDescriptor<&nox::test::service_node::SnIdleTask, nox::SystemPhaseType::FrameIngress>(
			"nox::test::service_node::SnAIdle");
	constexpr nox::UpdaterTaskDescriptor k_ingress_idle_b =
		nox::MakeUpdaterTaskDescriptor<&nox::test::service_node::SnOtherIdleTask, nox::SystemPhaseType::FrameIngress>(
			"nox::test::service_node::SnBIdle");
	constexpr nox::UpdaterTaskDescriptor k_ingress_idle_y =
		nox::MakeUpdaterTaskDescriptor<&nox::test::service_node::SnIdleTask, nox::SystemPhaseType::FrameIngress>(
			"nox::test::service_node::SnYIdle");
	constexpr nox::UpdaterTaskDescriptor k_ingress_idle_z =
		nox::MakeUpdaterTaskDescriptor<&nox::test::service_node::SnOtherIdleTask, nox::SystemPhaseType::FrameIngress>(
			"nox::test::service_node::SnZIdle");

	//	引数の規則。nox::World& は1フェーズに1回のノードでだけ、1つまで取れる。
	//	EntitySystem / EntityLogic の更新メソッドに書くと nox::detail::ValidateEntityMethod の static_assert で
	//	コンパイルエラーになる(ここではテストできないので、行単位の形として無効であることだけを見る)。
	static_assert(nox::EntitySignature<nox::World&>::k_world_parameter_count == 1u);
	static_assert(nox::EntitySignature<nox::World&>::k_is_valid_parameter_list);
	static_assert(nox::EntitySignature<nox::World&>::k_is_valid == false);
	static_assert(nox::EntitySignature<SnHealth&, nox::World&>::k_is_valid == false);
	static_assert(nox::EntitySignature<nox::World&, nox::World&>::k_is_valid_parameter_list == false);
	static_assert(nox::EntitySignature<const nox::World&>::k_all_parameters_valid == false);
	static_assert(nox::EntitySignature<nox::World*>::k_all_parameters_valid == false);
	static_assert(nox::detail::ValidateOncePerFrameSignature<
		nox::EntitySignature<nox::World&, SnAlphaService&, nox::EntityCommands&>>());
	//	World はServiceのアクセス宣言に算入しない(衝突は exclusive で表す)。
	static_assert(nox::EntitySignature<nox::World&, const SnAlphaService&>::k_service_parameter_count == 1u);

	/// @brief 指定フェーズを1回回す。World::ExecutePhase と同じ順(レイヤー順 → 反映)。
	void RunPhase(nox::World& world, const nox::UpdaterGraph& graph, nox::JobSystem& job_system, const nox::SystemPhaseType phase)
	{
		world.EnterEntityIteration();
		const nox::uint32 layer_count = graph.GetLayerCount(phase);
		for (nox::uint32 layer_index = 0u; layer_index < layer_count; ++layer_index)
		{
			nox::ExecuteUpdaterLayer(
				job_system,
				graph.GetLayerNodes(phase, layer_index),
				&ExecuteNodeForTest,
				&world);
		}
		world.LeaveEntityIteration();
		world.FlushEntityCommands();
	}
}

///	@brief	nox::World& を取るメソッド / Task は記述子で排他になり、属性が Any でもメインスレッド限定になる。
TEST(ServiceNode, WorldParameterMakesTheNodeExclusiveAndMainThreadOnly)
{
	const std::span<const nox::ServiceMethodDescriptor> methods = k_exclusive_descriptor.get_methods();
	ASSERT_EQ(methods.size(), 1u);
	EXPECT_EQ(methods[0].phase, nox::SystemPhaseType::FrameIngress);
	EXPECT_TRUE(methods[0].exclusive);
	EXPECT_TRUE(methods[0].main_thread_only);
	EXPECT_FALSE(methods[0].emits_structural_change);
	//	World はServiceのアクセス宣言に載らない。残るのは自己書き込みだけ。
	ASSERT_EQ(methods[0].get_service_accesses().size(), 1u);
	EXPECT_EQ(methods[0].get_service_accesses()[0].type, &nox::reflection::Typeof<SnExclusiveService>());

	EXPECT_TRUE(k_exclusive_task.exclusive);
	EXPECT_TRUE(k_exclusive_task.main_thread_only);
	EXPECT_TRUE(k_exclusive_task.get_service_accesses().empty());

	//	World を取らないノードは排他にならない。
	EXPECT_FALSE(k_ingress_idle_a.exclusive);
	EXPECT_FALSE(k_ingress_idle_a.main_thread_only);
	for (const nox::ServiceMethodDescriptor& method : k_alpha_descriptor.get_methods())
	{
		EXPECT_FALSE(method.exclusive) << method.name;
	}
}

///	@brief	排他は宣言の中身に関係なく全ノードと衝突する。前後のノードは排他ノードを挟んで別のレイヤーになる。
TEST(ServiceNode, ExclusiveAccessConflictsWithEveryNode)
{
	const nox::UpdaterNodeAccess empty{};
	const nox::UpdaterNodeAccess exclusive{ .exclusive = true };
	EXPECT_FALSE(nox::ConflictsUpdaterNodeAccess(empty, empty));
	EXPECT_TRUE(nox::ConflictsUpdaterNodeAccess(empty, exclusive));
	EXPECT_TRUE(nox::ConflictsUpdaterNodeAccess(exclusive, empty));
	EXPECT_TRUE(nox::ConflictsUpdaterNodeAccess(exclusive, exclusive));

	//	全順序: 空, 空, 排他, 空, 空 → レイヤー 0, 0, 1, 2, 2
	const std::array<nox::UpdaterNodeAccess, 5> accesses{ empty, empty, exclusive, empty, empty };
	std::array<nox::uint32, 5> layers{};
	EXPECT_EQ(nox::BuildUpdaterLayerIndices(
		std::span<const nox::UpdaterNodeAccess>(accesses),
		std::span<nox::uint32>(layers)), 3u);
	EXPECT_EQ(layers, (std::array<nox::uint32, 5>{ 0u, 0u, 1u, 2u, 2u }));
}

///	@brief	排他ノードは同じレイヤーに他のノードを置かない。排他でないノード同士は今までどおり同じレイヤーに並ぶ。
TEST(ServiceNode, ExclusiveNodeTakesALayerOfItsOwn)
{
	SnExclusiveService service;
	const std::array<nox::UpdaterServiceBinding, 1> bindings{
		nox::UpdaterServiceBinding{ .service = &service, .descriptor = &k_exclusive_descriptor },
	};
	//	並び(登録順)は結果に影響しない。
	const std::array<const nox::UpdaterTaskDescriptor*, 5> tasks{
		&k_ingress_idle_z, &k_exclusive_task, &k_ingress_idle_a, &k_ingress_idle_y, &k_ingress_idle_b,
	};

	nox::UpdaterGraph graph;
	const nox::UpdaterGraphBuildResult result = graph.TryRebuild(
		std::span<nox::EntitySystemBase* const>(),
		std::span<nox::EntityLogicStorage* const>(),
		std::span<const nox::UpdaterServiceBinding>(bindings.data(), bindings.size()),
		std::span<const nox::UpdaterTaskDescriptor* const>(tasks.data(), tasks.size()));
	ASSERT_TRUE(result.IsSuccess());

	//	FrameIngress にだけ載り、Update には何も無い。
	EXPECT_TRUE(graph.GetNodes(nox::SystemPhaseType::Update).empty());
	const std::span<const nox::UpdaterNode> nodes = graph.GetNodes(nox::SystemPhaseType::FrameIngress);
	ASSERT_EQ(nodes.size(), 6u);
	EXPECT_EQ(graph.GetLayerCount(nox::SystemPhaseType::FrameIngress), 4u);

	const nox::UpdaterNode* const idle_a = FindNode(nodes, k_ingress_idle_a.name);
	const nox::UpdaterNode* const idle_b = FindNode(nodes, k_ingress_idle_b.name);
	const nox::UpdaterNode* const ingress = FindNode(nodes, nox::util::GetTypeName<SnExclusiveService>(), "Ingress");
	const nox::UpdaterNode* const exclusive_task = FindNode(nodes, k_exclusive_task.name);
	const nox::UpdaterNode* const idle_y = FindNode(nodes, k_ingress_idle_y.name);
	const nox::UpdaterNode* const idle_z = FindNode(nodes, k_ingress_idle_z.name);
	ASSERT_NE(idle_a, nullptr);
	ASSERT_NE(idle_b, nullptr);
	ASSERT_NE(ingress, nullptr);
	ASSERT_NE(exclusive_task, nullptr);
	ASSERT_NE(idle_y, nullptr);
	ASSERT_NE(idle_z, nullptr);

	//	排他ノードは access.exclusive と main_thread_only が立つ。
	EXPECT_TRUE(ingress->access.exclusive);
	EXPECT_TRUE(ingress->main_thread_only);
	EXPECT_TRUE(exclusive_task->access.exclusive);
	EXPECT_TRUE(exclusive_task->main_thread_only);
	EXPECT_FALSE(idle_a->access.exclusive);
	EXPECT_FALSE(idle_a->main_thread_only);

	//	全順序は型名 / 関数名の順。排他ノードの位置も同じ規則で決まる。
	EXPECT_LT(idle_b->order_index, ingress->order_index);
	EXPECT_LT(ingress->order_index, exclusive_task->order_index);
	EXPECT_LT(exclusive_task->order_index, idle_y->order_index);

	//	前の 2 つは同じレイヤー、排他ノードはそれぞれ単独、後ろの 2 つも同じレイヤー。
	EXPECT_EQ(idle_a->layer_index, 0u);
	EXPECT_EQ(idle_b->layer_index, 0u);
	EXPECT_EQ(ingress->layer_index, 1u);
	EXPECT_EQ(exclusive_task->layer_index, 2u);
	EXPECT_EQ(idle_y->layer_index, 3u);
	EXPECT_EQ(idle_z->layer_index, 3u);
	EXPECT_EQ(graph.GetLayerNodes(nox::SystemPhaseType::FrameIngress, 1u).size(), 1u);
	EXPECT_EQ(graph.GetLayerNodes(nox::SystemPhaseType::FrameIngress, 2u).size(), 1u);

	for (const nox::UpdaterNode& node : nodes)
	{
		if (node.access.exclusive == false)
		{
			continue;
		}
		for (const nox::UpdaterNode& other : nodes)
		{
			if (&other != &node)
			{
				EXPECT_NE(other.layer_index, node.layer_index) << nox::GetUpdaterNodeTypeName(other);
				EXPECT_TRUE(nox::ConflictsUpdaterNodeAccess(node.access, other.access));
			}
		}
	}
	EXPECT_FALSE(nox::ConflictsUpdaterNodeAccess(idle_a->access, idle_b->access));
}

///	@brief	World& を取るメソッドと Task は、ワーカーがあってもフェーズごとにちょうど1回、呼び出しスレッドで、
///			フェーズを回している World を受け取って呼ばれる。
TEST(ServiceNode, ExclusiveNodesRunOncePerPhaseOnTheCallerThreadWithTheWorld)
{
	static constexpr nox::int32 kFrameCount = 8;

	nox::World world;
	world.RegisterService(*new SnExclusiveService());
	ASSERT_TRUE(world.TryInitializeServices().IsSuccess());
	SnExclusiveService* const service = world.TryGetService<SnExclusiveService>();
	ASSERT_NE(service, nullptr);

	const std::array<nox::UpdaterServiceBinding, 1> bindings{
		nox::UpdaterServiceBinding{ .service = service, .descriptor = &k_exclusive_descriptor },
	};
	const std::array<const nox::UpdaterTaskDescriptor*, 5> tasks{
		&k_ingress_idle_a, &k_ingress_idle_b, &k_exclusive_task, &k_ingress_idle_y, &k_ingress_idle_z,
	};

	nox::UpdaterGraph graph;
	const nox::UpdaterGraphBuildResult result = graph.TryRebuild(
		std::span<nox::EntitySystemBase* const>(),
		std::span<nox::EntityLogicStorage* const>(),
		std::span<const nox::UpdaterServiceBinding>(bindings.data(), bindings.size()),
		std::span<const nox::UpdaterTaskDescriptor* const>(tasks.data(), tasks.size()));
	ASSERT_TRUE(result.IsSuccess());
	world.ReserveNodeEntityCommandBuffers(graph.GetCommandBufferCount(nox::SystemPhaseType::FrameIngress));

	g_caller_thread_id = std::this_thread::get_id();
	g_idle_task_calls.store(0, std::memory_order_relaxed);
	g_other_idle_task_calls.store(0, std::memory_order_relaxed);
	g_exclusive_task_calls.store(0, std::memory_order_relaxed);
	g_exclusive_task_on_caller_every_time.store(true, std::memory_order_relaxed);

	nox::JobSystem job_system;
	job_system.Initialize(2u);
	for (nox::int32 frame = 0; frame < kFrameCount; ++frame)
	{
		RunPhase(world, graph, job_system, nox::SystemPhaseType::FrameIngress);
	}
	job_system.Finalize();

	EXPECT_EQ(service->ingress_count, kFrameCount);
	EXPECT_EQ(service->last_world, &world);
	EXPECT_TRUE(service->ingress_on_caller_every_time);
	EXPECT_EQ(g_exclusive_task_calls.load(std::memory_order_relaxed), kFrameCount);
	EXPECT_TRUE(g_exclusive_task_on_caller_every_time.load(std::memory_order_relaxed));
	//	排他でない Task は2つの記述子から呼ばれる(A と Y が SnIdleTask、B と Z が SnOtherIdleTask)。
	EXPECT_EQ(g_idle_task_calls.load(std::memory_order_relaxed), kFrameCount * 2);
	EXPECT_EQ(g_other_idle_task_calls.load(std::memory_order_relaxed), kFrameCount * 2);
}
