//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	service_phase_test.cpp
///	@brief	Service のフェーズ関数の検証。記述子・World への配置・UpdaterGraph への組み込み。
///	@details	検証は4層に分かれている。
///
///				1. 記述子 (nox::kServiceTypeDescriptor)
///				   kPhaseList の並び・フェーズ・関数名・読み書きの宣言(自分自身を含む)・
///				   After / Before のキーが、宣言どおりに導出されること。
///				2. World への配置 (nox::World::CreateServices)
///				   1つの領域へアラインメントを守って並び、型で引け、生成と逆の順で破棄されること。
///				3. 順序の規則 (nox::SortUpdaterNodeOrder / nox::BuildUpdaterLayerIndices)
///				   指定の無いノードは元の順を保ち、指定された先行ノードは前倒しされること。循環を検出すること。
///				4. 本番の構築経路 (nox::UpdaterGraph::Rebuild)
///				   引数が構築時に解決され、After / Before と System との衝突で実行順が決まること。
///
///	@note		テスト用の Service はこの .cpp に閉じてあり、リフレクション生成コードからは見えない
///				(World には購読されない)。生成器の表に載る経路は nox::test::ecs::TestTickService で確かめる。
///				World::Init は private なので、World へは CreateServices で直接置き、
///				UpdaterGraph はテストが組んでノードを順に呼ぶ(updater_graph_layering_test.cpp と同じ方針)。

#include	"pch.h"

#include	<algorithm>

//	core_test の pch.h は gtest しか載せていない。core のヘッダは kernel / reflection の基盤型に依存するので先に入れる。
#include	"../../kernel/kernel.h"
#include	"../../reflection/reflection.h"

#include	"entity_ecs_test.h"
#include	"../world.h"
#include	"../service.h"
#include	"../updater_graph.h"
#include	"../entity_system_legacy.h"
#include	"../entity_type_registry.h"

namespace nox::test::service_phase
{
	/// @brief フェーズ関数とデストラクタが呼ばれた順の記録。テストごとに Reset する。
	struct CallRecorder final
	{
		std::array<char, 16> calls{};
		nox::uint32 count = 0u;

		void Reset()noexcept
		{
			count = 0u;
		}

		void Record(const char call)noexcept
		{
			if (count < calls.size())
			{
				calls[count] = call;
				++count;
			}
		}

		[[nodiscard]] std::string_view View()const noexcept
		{
			return std::string_view(calls.data(), count);
		}
	};

	inline CallRecorder g_recorder{};
	inline bool g_report_received_null = false;

	/// @brief 毎フレーム進む時刻。依存される側。
	class TimeService final : public nox::Service<TimeService>
	{
		NOX_ECS_DECLARE_VERIFY(TimeService);

	private:
		void Tick()
		{
			++tick_count;
			g_recorder.Record('T');
		}

	public:
		static constexpr auto kTickPhase = PhaseUpdate<&TimeService::Tick>{};
		static constexpr auto kPhaseList = PhaseRegister{ kTickPhase };

		nox::int32 tick_count = 0;
	};

	/// @brief フェーズを持たない Service。引数として受けられるだけ。
	class ConfigService final : public nox::Service<ConfigService>
	{
		NOX_ECS_DECLARE_VERIFY(ConfigService);

	public:
		static constexpr auto kPhaseList = PhaseRegister{};

		nox::int32 limit = 3;
	};

	/// @brief World に置かない Service。ポインタで受けると nullptr が渡ることを確かめる。
	class MissingService final : public nox::Service<MissingService>
	{
		NOX_ECS_DECLARE_VERIFY(MissingService);

	public:
		static constexpr auto kPhaseList = PhaseRegister{};
	};

	/// @brief Init / Update / Terminate と After、const メンバ関数、ポインタ引数を一通り使う見本。
	class SampleService final : public nox::Service<SampleService>
	{
		NOX_ECS_DECLARE_VERIFY(SampleService);

	private:
		void Setup(const ConfigService& config)
		{
			limit = config.limit;
			g_recorder.Record('S');
		}

		void Tick(const TimeService& time)
		{
			observed_tick_count = time.tick_count;
			g_recorder.Record('t');
		}

		void Report(MissingService* missing)const
		{
			g_report_received_null = (missing == nullptr);
			g_recorder.Record('r');
		}

		void Shutdown()
		{
			g_recorder.Record('X');
		}

	public:
		static constexpr auto kSetupPhase = PhaseInit<&SampleService::Setup>{};
		//	TimeService の Tick より後に実行する。
		static constexpr auto kTickPhase = PhaseUpdate<&SampleService::Tick, After<TimeService::kTickPhase>>{};
		static constexpr auto kReportPhase = PhaseUpdate<&SampleService::Report>{};
		static constexpr auto kShutdownPhase = PhaseTerminate<&SampleService::Shutdown>{};
		static constexpr auto kPhaseList = PhaseRegister{ kSetupPhase, kTickPhase, kReportPhase, kShutdownPhase };

		nox::int32 limit = 0;
		nox::int32 observed_tick_count = -1;
	};

	/// @brief TimeService の Tick より前に実行すると Before で宣言する。宣言そのものは TimeService と衝突しない。
	class EarlyService final : public nox::Service<EarlyService>
	{
		NOX_ECS_DECLARE_VERIFY(EarlyService);

	private:
		void Tick()
		{
			g_recorder.Record('E');
		}

	public:
		static constexpr auto kTickPhase = PhaseUpdate<&EarlyService::Tick, Before<TimeService::kTickPhase>>{};
		static constexpr auto kPhaseList = PhaseRegister{ kTickPhase };
	};

	/// @brief 破棄の順を記録する Service(1つ目)。
	class FirstDestroyService final : public nox::Service<FirstDestroyService>
	{
		NOX_ECS_DECLARE_VERIFY(FirstDestroyService);

	public:
		static constexpr auto kPhaseList = PhaseRegister{};

		~FirstDestroyService()
		{
			g_recorder.Record('1');
		}
	};

	/// @brief 破棄の順を記録する Service(2つ目)。アラインメントの違う配置も兼ねる。
	class SecondDestroyService final : public nox::Service<SecondDestroyService>
	{
		NOX_ECS_DECLARE_VERIFY(SecondDestroyService);

	public:
		static constexpr auto kPhaseList = PhaseRegister{};

		~SecondDestroyService()
		{
			g_recorder.Record('2');
		}

		alignas(64) nox::uint8 payload[64]{};
	};

	/// @brief Update で TimeService を読む System。TimeService の Tick(書き込み)と衝突する。
	class TimeReaderSystem final : public nox::legacy::EntitySystem<nox::test::service_phase::TimeReaderSystem>
	{
	public:
		void OnUpdate(const nox::test::service_phase::TimeService&)
		{
		}
	};

	/// @brief Terminate で SampleService を読む System。SampleService の Shutdown(書き込み)と衝突する。
	class SampleReaderSystem final
		: public nox::legacy::EntitySystem<nox::test::service_phase::SampleReaderSystem, nox::SystemPhaseType::Terminate>
	{
	public:
		void OnUpdate(const nox::test::service_phase::SampleService&)
		{
		}
	};

	/// @brief グラフのノードを Service の型名とフェーズ関数の名前で探す。
	[[nodiscard]] const nox::UpdaterNode* FindServiceNode(
		const nox::UpdaterGraph& graph,
		const nox::SystemPhaseType phase_type,
		const std::string_view service_name,
		const std::string_view method_name)noexcept
	{
		for (const nox::UpdaterNode& node : graph.GetNodes(phase_type))
		{
			if (node.kind != nox::UpdaterNodeKind::ServicePhaseMethod)
			{
				continue;
			}
			if ((node.service_method->service_name.find(service_name) != std::string_view::npos) &&
				(node.service_method->name == method_name))
			{
				return &node;
			}
		}
		return nullptr;
	}

	[[nodiscard]] const nox::UpdaterNode* FindSystemNode(
		const nox::UpdaterGraph& graph,
		const nox::SystemPhaseType phase_type,
		const nox::legacy::EntitySystemBase* const system)noexcept
	{
		for (const nox::UpdaterNode& node : graph.GetNodes(phase_type))
		{
			if ((node.kind == nox::UpdaterNodeKind::EntitySystem) && (node.system == system))
			{
				return &node;
			}
		}
		return nullptr;
	}

	/// @brief フェーズの Service のノードを、World と同じくレイヤー順に呼ぶ。
	void InvokeServiceNodes(const nox::UpdaterGraph& graph, const nox::SystemPhaseType phase_type)
	{
		for (nox::uint32 layer_index = 0u; layer_index < graph.GetLayerCount(phase_type); ++layer_index)
		{
			for (const nox::UpdaterNode& node : graph.GetLayerNodes(phase_type, layer_index))
			{
				if (node.kind == nox::UpdaterNodeKind::ServicePhaseMethod)
				{
					node.service_method->invoke(node.service_instance, node.service_arguments);
				}
			}
		}
	}
}

using namespace nox::test::service_phase;

//	=====================================================================================
//	1. 記述子
//	=====================================================================================

///	@brief	kPhaseList の並びどおりに、フェーズ・関数名・型情報が導出される。
TEST(ServicePhaseDescriptor, FollowsPhaseList)
{
	const nox::ServiceTypeDescriptor& descriptor = nox::kServiceTypeDescriptor<SampleService>;
	const std::span<const nox::ServicePhaseMethodDescriptor> methods = descriptor.get_phase_methods();
	ASSERT_EQ(methods.size(), 4u);

	EXPECT_TRUE(methods[0].phase == nox::SystemPhaseType::Init);
	EXPECT_TRUE(methods[1].phase == nox::SystemPhaseType::Update);
	EXPECT_TRUE(methods[2].phase == nox::SystemPhaseType::Update);
	EXPECT_TRUE(methods[3].phase == nox::SystemPhaseType::Terminate);

	EXPECT_EQ(methods[0].name, std::string_view("Setup"));
	EXPECT_EQ(methods[1].name, std::string_view("Tick"));
	EXPECT_EQ(methods[2].name, std::string_view("Report"));
	EXPECT_EQ(methods[3].name, std::string_view("Shutdown"));
	EXPECT_NE(methods[1].service_name.find("SampleService"), std::string_view::npos);

	EXPECT_EQ(descriptor.type, &nox::reflection::Typeof<SampleService>());
	EXPECT_EQ(descriptor.instance_size, static_cast<nox::uint32>(sizeof(SampleService)));
	EXPECT_EQ(descriptor.instance_alignment, static_cast<nox::uint32>(alignof(SampleService)));

	//	フェーズを持たない Service は空の一覧になる。
	EXPECT_TRUE(nox::kServiceTypeDescriptor<ConfigService>.get_phase_methods().empty());
}

///	@brief	引数が読み書きの宣言になり、自分自身への読み書きは const 修飾から暗黙に入る。
TEST(ServicePhaseDescriptor, DeclaresSelfAndParameterAccesses)
{
	const std::span<const nox::ServicePhaseMethodDescriptor> methods =
		nox::kServiceTypeDescriptor<SampleService>.get_phase_methods();

	//	Tick(const TimeService&) は非 const: 自分へ書き込み、TimeService を読む。
	const std::span<const nox::ServiceAccess> tick_accesses = methods[1].get_service_accesses();
	ASSERT_EQ(tick_accesses.size(), 2u);
	EXPECT_EQ(tick_accesses[0].type, &nox::reflection::Typeof<SampleService>());
	EXPECT_TRUE(tick_accesses[0].write);
	EXPECT_EQ(tick_accesses[1].type, &nox::reflection::Typeof<TimeService>());
	EXPECT_FALSE(tick_accesses[1].write);

	//	Report(MissingService*) const: 自分は読み取り、非 const のポインタで受けた MissingService へは書き込み。
	const std::span<const nox::ServiceAccess> report_accesses = methods[2].get_service_accesses();
	ASSERT_EQ(report_accesses.size(), 2u);
	EXPECT_FALSE(report_accesses[0].write);
	EXPECT_EQ(report_accesses[1].type, &nox::reflection::Typeof<MissingService>());
	EXPECT_TRUE(report_accesses[1].write);

	//	参照で受けた引数は必須、ポインタで受けた引数は無くてもよい。
	ASSERT_EQ(methods[1].get_parameters().size(), 1u);
	EXPECT_TRUE(methods[1].get_parameters()[0].required);
	ASSERT_EQ(methods[2].get_parameters().size(), 1u);
	EXPECT_FALSE(methods[2].get_parameters()[0].required);

	//	引数の無いフェーズ関数は、自分自身への宣言だけを持つ。
	ASSERT_EQ(methods[3].get_service_accesses().size(), 1u);
	EXPECT_TRUE(methods[3].get_parameters().empty());
}

///	@brief	After / Before は、指した先のフェーズのキー(ハンドルの型情報)になる。
TEST(ServicePhaseDescriptor, DependenciesPointToOtherHandles)
{
	const nox::ServicePhaseMethodDescriptor& sample_tick = nox::kServiceTypeDescriptor<SampleService>.get_phase_methods()[1];
	const nox::ServicePhaseMethodDescriptor& time_tick = nox::kServiceTypeDescriptor<TimeService>.get_phase_methods()[0];
	const nox::ServicePhaseMethodDescriptor& early_tick = nox::kServiceTypeDescriptor<EarlyService>.get_phase_methods()[0];

	ASSERT_EQ(sample_tick.get_after_phases().size(), 1u);
	EXPECT_EQ(sample_tick.get_after_phases()[0], time_tick.phase_key);
	EXPECT_TRUE(sample_tick.get_before_phases().empty());

	ASSERT_EQ(early_tick.get_before_phases().size(), 1u);
	EXPECT_EQ(early_tick.get_before_phases()[0], time_tick.phase_key);
	EXPECT_TRUE(early_tick.get_after_phases().empty());

	//	ハンドルが違えばキーも違う。
	EXPECT_NE(sample_tick.phase_key, time_tick.phase_key);
	EXPECT_NE(early_tick.phase_key, time_tick.phase_key);
}

///	@brief	System / EntityLogic の引数にも新しい Service を書ける(読み書きの宣言になる)。
TEST(ServicePhaseDescriptor, EntitySignatureAcceptsNewService)
{
	using Signature = nox::EntitySignature<const TimeService&, SampleService*>;
	static_assert(Signature::k_is_valid);
	static_assert(Signature::k_service_parameter_count == 2u);

	const std::span<const nox::ServiceAccess> accesses = Signature::GetServiceAccesses();
	ASSERT_EQ(accesses.size(), 2u);
	EXPECT_EQ(accesses[0].type, &nox::reflection::Typeof<TimeService>());
	EXPECT_FALSE(accesses[0].write);
	EXPECT_EQ(accesses[1].type, &nox::reflection::Typeof<SampleService>());
	EXPECT_TRUE(accesses[1].write);
}

///	@brief	ヘッダに定義しただけの Service が、生成器の表に載る。
TEST(ServicePhaseDescriptor, GeneratedTableListsSubscribedService)
{
	const std::span<const nox::ServiceTypeDescriptor* const> service_types = nox::GetServiceTypes();
	const nox::ServiceTypeDescriptor* const expected = &nox::kServiceTypeDescriptor<nox::test::ecs::TestTickService>;
	EXPECT_NE(std::ranges::find(service_types, expected), service_types.end());

	//	.cpp に閉じた Service は生成器から見えないので載らない。
	const nox::ServiceTypeDescriptor* const hidden = &nox::kServiceTypeDescriptor<SampleService>;
	EXPECT_EQ(std::ranges::find(service_types, hidden), service_types.end());
}

//	=====================================================================================
//	2. World への配置
//	=====================================================================================

///	@brief	Service は1つの領域へ並び、型で引ける。並べなかった型は引けない。
TEST(ServiceWorld, CreateServicesPlacesEachServiceOnce)
{
	nox::World world;
	const std::array<const nox::ServiceTypeDescriptor*, 4> service_types{
		&nox::kServiceTypeDescriptor<SampleService>,
		&nox::kServiceTypeDescriptor<TimeService>,
		&nox::kServiceTypeDescriptor<ConfigService>,
		&nox::kServiceTypeDescriptor<SecondDestroyService>,
	};
	world.CreateServices(service_types);

	SampleService* const sample = world.TryGetService<SampleService>();
	TimeService* const time = world.TryGetService<TimeService>();
	ConfigService* const config = world.TryGetService<ConfigService>();
	SecondDestroyService* const aligned = world.TryGetService<SecondDestroyService>();
	ASSERT_NE(sample, nullptr);
	ASSERT_NE(time, nullptr);
	ASSERT_NE(config, nullptr);
	ASSERT_NE(aligned, nullptr);

	//	デフォルト構築(メンバ初期化子)された状態で置かれる。
	EXPECT_EQ(sample->observed_tick_count, -1);
	EXPECT_EQ(config->limit, 3);

	//	アラインメントは型ごとに守られる。
	EXPECT_EQ(reinterpret_cast<std::uintptr_t>(sample) % alignof(SampleService), 0u);
	EXPECT_EQ(reinterpret_cast<std::uintptr_t>(aligned) % alignof(SecondDestroyService), 0u);

	EXPECT_EQ(world.TryGetService<MissingService>(), nullptr);
	EXPECT_EQ(world.GetServices().size(), 4u);
	EXPECT_EQ(world.TryGetServiceInstance(nox::reflection::Typeof<TimeService>()), static_cast<void*>(time));
}

///	@brief	World の破棄で、生成と逆の順に破棄される。
TEST(ServiceWorld, DestroysInReverseOrder)
{
	g_recorder.Reset();
	{
		nox::World world;
		const std::array<const nox::ServiceTypeDescriptor*, 2> service_types{
			&nox::kServiceTypeDescriptor<FirstDestroyService>,
			&nox::kServiceTypeDescriptor<SecondDestroyService>,
		};
		world.CreateServices(service_types);
		EXPECT_EQ(g_recorder.View(), std::string_view());
	}
	EXPECT_EQ(g_recorder.View(), std::string_view("21"));
}

//	=====================================================================================
//	3. 順序の規則
//	=====================================================================================

///	@brief	指定が無ければ元の順のまま。
TEST(UpdaterNodeOrder, KeepsRegistrationOrderWithoutEdges)
{
	std::array<nox::uint32, 4> order{};
	std::array<nox::uint8, 4> state{};
	EXPECT_TRUE(nox::SortUpdaterNodeOrder(std::span<const nox::UpdaterNodeOrderEdge>(), order, state));
	EXPECT_EQ(order, (std::array<nox::uint32, 4>{ 0u, 1u, 2u, 3u }));
}

///	@brief	先行ノードは、それを必要とするノードの直前へ前倒しされる。他のノードは元の順を保つ。
TEST(UpdaterNodeOrder, HoistsPredecessorsAndKeepsTheRest)
{
	//	2 を 0 より先に、3 を 2 より先に。1 には指定が無い。
	const std::array<nox::UpdaterNodeOrderEdge, 2> edges{
		nox::UpdaterNodeOrderEdge{ .from = 2u, .to = 0u },
		nox::UpdaterNodeOrderEdge{ .from = 3u, .to = 2u },
	};
	std::array<nox::uint32, 4> order{};
	std::array<nox::uint8, 4> state{};
	EXPECT_TRUE(nox::SortUpdaterNodeOrder(edges, order, state));
	EXPECT_EQ(order, (std::array<nox::uint32, 4>{ 3u, 2u, 0u, 1u }));
}

///	@brief	循環は検出され、結果は全ノードを1回ずつ含む並びのまま返る。
TEST(UpdaterNodeOrder, DetectsCycle)
{
	const std::array<nox::UpdaterNodeOrderEdge, 2> edges{
		nox::UpdaterNodeOrderEdge{ .from = 0u, .to = 1u },
		nox::UpdaterNodeOrderEdge{ .from = 1u, .to = 0u },
	};
	std::array<nox::uint32, 3> order{};
	std::array<nox::uint8, 3> state{};
	EXPECT_FALSE(nox::SortUpdaterNodeOrder(edges, order, state));

	std::array<nox::uint32, 3> sorted = order;
	std::ranges::sort(sorted);
	EXPECT_EQ(sorted, (std::array<nox::uint32, 3>{ 0u, 1u, 2u }));
}

///	@brief	順序の指定は、衝突しないノード同士でも別のレイヤーへ分ける。
TEST(UpdaterNodeOrder, EdgeSeparatesNonConflictingNodes)
{
	const std::array<nox::UpdaterNodeAccess, 2> accesses{};
	std::array<nox::uint32, 2> layers{};

	EXPECT_EQ(nox::BuildUpdaterLayerIndices(accesses, layers), 1u);
	EXPECT_EQ(layers, (std::array<nox::uint32, 2>{ 0u, 0u }));

	const std::array<nox::UpdaterNodeOrderEdge, 1> edges{ nox::UpdaterNodeOrderEdge{ .from = 0u, .to = 1u } };
	EXPECT_EQ(nox::BuildUpdaterLayerIndices(accesses, edges, layers), 2u);
	EXPECT_EQ(layers, (std::array<nox::uint32, 2>{ 0u, 1u }));
}

//	=====================================================================================
//	4. 本番の構築経路
//	=====================================================================================

///	@brief	After / Before で実行順が決まり、引数は構築時に解決される。
TEST(ServiceGraph, DependenciesOrderPhasesAndArgumentsAreResolved)
{
	g_recorder.Reset();
	g_report_received_null = false;

	nox::World world;
	//	既定の登録順では SampleService が TimeService より先になる並びで置く。
	const std::array<const nox::ServiceTypeDescriptor*, 4> service_types{
		&nox::kServiceTypeDescriptor<SampleService>,
		&nox::kServiceTypeDescriptor<TimeService>,
		&nox::kServiceTypeDescriptor<ConfigService>,
		&nox::kServiceTypeDescriptor<EarlyService>,
	};
	world.CreateServices(service_types);

	nox::UpdaterGraph graph;
	graph.Rebuild({}, {}, world.GetServices());

	const nox::UpdaterNode* const early_tick = FindServiceNode(graph, nox::SystemPhaseType::Update, "EarlyService", "Tick");
	const nox::UpdaterNode* const time_tick = FindServiceNode(graph, nox::SystemPhaseType::Update, "TimeService", "Tick");
	const nox::UpdaterNode* const sample_tick = FindServiceNode(graph, nox::SystemPhaseType::Update, "SampleService", "Tick");
	const nox::UpdaterNode* const sample_report = FindServiceNode(graph, nox::SystemPhaseType::Update, "SampleService", "Report");
	ASSERT_NE(early_tick, nullptr);
	ASSERT_NE(time_tick, nullptr);
	ASSERT_NE(sample_tick, nullptr);
	ASSERT_NE(sample_report, nullptr);

	//	Before: EarlyService は TimeService と衝突しないが、指定どおり前のレイヤーへ載る。
	EXPECT_LT(early_tick->layer_index, time_tick->layer_index);
	//	After: TimeService は既定の登録順では後ろだったが、SampleService の Tick より前へ前倒しされる。
	EXPECT_LT(time_tick->order_index, sample_tick->order_index);
	EXPECT_LT(time_tick->layer_index, sample_tick->layer_index);
	//	同じ Service の中の並び(kPhaseList の順)は保たれる。
	EXPECT_LT(sample_tick->order_index, sample_report->order_index);

	InvokeServiceNodes(graph, nox::SystemPhaseType::Init);
	InvokeServiceNodes(graph, nox::SystemPhaseType::Update);
	InvokeServiceNodes(graph, nox::SystemPhaseType::Terminate);

	const SampleService* const sample = world.TryGetService<SampleService>();
	ASSERT_NE(sample, nullptr);
	//	参照で受けた引数は World の実体に解決されている。
	EXPECT_EQ(sample->limit, 3);
	EXPECT_EQ(sample->observed_tick_count, 1);
	//	World に無い Service をポインタで受けると nullptr が渡る。
	EXPECT_TRUE(g_report_received_null);
	EXPECT_EQ(g_recorder.View(), std::string_view("SETtrX"));
}

///	@brief	System と衝突したとき、Update では Service が先、Terminate では Service が後に回る。
TEST(ServiceGraph, ServicesPrecedeSystemsExceptTerminate)
{
	nox::World world;
	const std::array<const nox::ServiceTypeDescriptor*, 3> service_types{
		&nox::kServiceTypeDescriptor<SampleService>,
		&nox::kServiceTypeDescriptor<TimeService>,
		&nox::kServiceTypeDescriptor<ConfigService>,
	};
	world.CreateServices(service_types);

	TimeReaderSystem time_reader;
	SampleReaderSystem sample_reader;
	const std::array<nox::legacy::EntitySystemBase*, 2> systems{ &time_reader, &sample_reader };

	nox::UpdaterGraph graph;
	graph.Rebuild(systems, {}, world.GetServices());

	const nox::UpdaterNode* const time_tick = FindServiceNode(graph, nox::SystemPhaseType::Update, "TimeService", "Tick");
	const nox::UpdaterNode* const time_reader_node = FindSystemNode(graph, nox::SystemPhaseType::Update, &time_reader);
	ASSERT_NE(time_tick, nullptr);
	ASSERT_NE(time_reader_node, nullptr);
	EXPECT_LT(time_tick->layer_index, time_reader_node->layer_index);

	const nox::UpdaterNode* const shutdown = FindServiceNode(graph, nox::SystemPhaseType::Terminate, "SampleService", "Shutdown");
	const nox::UpdaterNode* const sample_reader_node = FindSystemNode(graph, nox::SystemPhaseType::Terminate, &sample_reader);
	ASSERT_NE(shutdown, nullptr);
	ASSERT_NE(sample_reader_node, nullptr);
	EXPECT_LT(sample_reader_node->layer_index, shutdown->layer_index);

	//	Service のフェーズ関数は遅延構造変更を出さないので、コマンドバッファを割り当てない。
	EXPECT_EQ(time_tick->command_buffer_index, nox::k_invalid_updater_command_buffer_index);
}
