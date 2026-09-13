//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	updater_graph_layering_test.cpp
///	@brief	UpdaterGraph の衝突判定とレイヤリングの検証。時間には一切依存しない。
///	@details	UpdaterGraph の半自動並列化は「衝突しないノードを同一レイヤーへ載せ、
///				そのレイヤーをまとめてワーカーへ配る」で成立している。
///				つまり **衝突判定が正しいこと** が並列実行の安全性そのものであり、
///				**同一レイヤーに2つ以上のノードが載ること** が並列ディスパッチの
///				コードパスが踏まれる前提条件になる。
///				どちらもこれまで検証されていなかったので、ここで固定する。
///
///				検証は3層に分かれている。
///
///				1. 規則そのもの (nox::ConflictsUpdaterNodeAccess)
///				   ComponentData の read/write、Service の read/write、
///				   EntityLogic のインスタンス状態共有 (group_index) の3系統。
///				2. レイヤリング (nox::BuildUpdaterLayerIndices)
///				   規則からレイヤー番号が導かれること。独立なノードが同一レイヤーへ載ること。
///				3. 本番の構築経路 (nox::UpdaterGraph::Rebuild)
///				   EntitySystem / EntityLogic の「引数リスト」からアクセス宣言が導出され、
///				   1 と 2 が実型に対しても成立すること。
///
///				最後に 3 で組んだ実グラフに対して不変条件
///				  ・同一レイヤーの任意のノード対は衝突しない
///				  ・衝突するノード対は必ずレイヤーが真に増加する (登録順が保たれる)
///				を全数検査する。ここが「たまたま通っている」を排除する本体で、
///				衝突判定を壊すと必ず落ちる。
///
///	@note		World は使わない。nox::World::Init() は private で、core_test から
///				本番のフェーズ実行を駆動できないため。
///				nox::UpdaterGraph::Rebuild / nox::BuildUpdaterLayerIndices /
///				nox::ConflictsUpdaterNodeAccess はいずれも public かつ World 非依存で、
///				updater_graph.h にも「Worldを介さずに組み立てられるため、テストから直接
///				レイヤリングを検証できる」と明記されている。World の公開範囲は広げていない。

#include	"pch.h"

//	core_test の pch.h は gtest しか載せていない (test_new_delete.cpp の都合)。
//	core のヘッダは kernel / reflection の基盤型に依存するので、main.cpp と同じ順で先に入れる。
#include	"../../kernel/kernel.h"
#include	"../../reflection/reflection.h"

#include	"../updater_graph.h"
#include	"../entity_system.h"
#include	"../entity_logic.h"
#include	"../service.h"

namespace nox::test::updater_graph
{
	//	=================================================================================
	//	テスト用の ComponentData / Service
	//	=================================================================================

	struct LayerA : nox::IComponentData { nox::float32 value; };
	struct LayerB : nox::IComponentData { nox::float32 value; };
	struct LayerC : nox::IComponentData { nox::float32 value; };
	struct LayerD : nox::IComponentData { nox::float32 value; };
	//	Service の規則だけを見るための、他のどのノードとも重ならない ComponentData。
	//	ComponentData を共有させてしまうと「Service で分かれた」のか
	//	「ComponentData で分かれた」のかが区別できない。
	struct LayerE : nox::IComponentData { nox::float32 value; };
	struct LayerF : nox::IComponentData { nox::float32 value; };
	struct LayerG : nox::IComponentData { nox::float32 value; };

	/// @brief Service の同一性は型情報のアドレスで見るので、中身は要らない。
	class LayerServiceX final : public nox::Service {};
	class LayerServiceY final : public nox::Service {};

	//	=================================================================================
	//	テスト用の EntitySystem
	//	宣言 (= OnUpdate の引数リスト) だけが依存解析の入力になる。
	//	=================================================================================

	/// @brief LayerA へ書き込む。
	class WriteASystem final : public nox::EntitySystem<nox::test::updater_graph::WriteASystem>
	{
	public:
		void OnUpdate(nox::test::updater_graph::LayerA& a) { a.value += 1.0f; }
	};

	/// @brief LayerB へ書き込む。WriteASystem とは宣言が重ならない。
	class WriteBSystem final : public nox::EntitySystem<nox::test::updater_graph::WriteBSystem>
	{
	public:
		void OnUpdate(nox::test::updater_graph::LayerB& b) { b.value += 1.0f; }
	};

	/// @brief LayerA を読むだけ。読み同士は衝突しない。
	class ReadASystem final : public nox::EntitySystem<nox::test::updater_graph::ReadASystem>
	{
	public:
		void OnUpdate(const nox::test::updater_graph::LayerA& a, nox::test::updater_graph::LayerC& c)
		{
			c.value = a.value;
		}
	};

	/// @brief LayerA を読むだけ (別の型)。ReadASystem と同一レイヤーへ載るべき。
	class ReadA2System final : public nox::EntitySystem<nox::test::updater_graph::ReadA2System>
	{
	public:
		void OnUpdate(const nox::test::updater_graph::LayerA& a, nox::test::updater_graph::LayerD& d)
		{
			d.value = a.value;
		}
	};

	/// @brief Service を書き込みで受ける。
	class ServiceWriteSystem final : public nox::EntitySystem<nox::test::updater_graph::ServiceWriteSystem>
	{
	public:
		void OnUpdate(nox::test::updater_graph::LayerE& e, nox::test::updater_graph::LayerServiceX* service)
		{
			e.value += (service != nullptr) ? 1.0f : 0.0f;
		}
	};

	/// @brief 同じ Service を読み取りで受ける。ComponentData は誰とも重ならない。
	class ServiceReadSystem final : public nox::EntitySystem<nox::test::updater_graph::ServiceReadSystem>
	{
	public:
		void OnUpdate(nox::test::updater_graph::LayerF& f, const nox::test::updater_graph::LayerServiceX* service)
		{
			f.value += (service != nullptr) ? 1.0f : 0.0f;
		}
	};

	/// @brief 別の Service を読み取りで受ける。上の2つのどちらとも衝突しない。
	class OtherServiceReadSystem final : public nox::EntitySystem<nox::test::updater_graph::OtherServiceReadSystem>
	{
	public:
		void OnUpdate(nox::test::updater_graph::LayerG& g, const nox::test::updater_graph::LayerServiceY* service)
		{
			g.value += (service != nullptr) ? 1.0f : 0.0f;
		}
	};

	//	=================================================================================
	//	テスト用の EntityLogic
	//	更新メソッドの購読は通常リフレクション生成コードが行うが、
	//	nox::EntityLogicMethodTable の手書き特殊化 (entity_logic.h が明記している
	//	エスケープハッチ) を使えば生成器を通さずに同じ経路へ載せられる。
	//	core_test 専用の型を core/test_support/test_types.h へ足さずに済み、
	//	Master でテスト型のリフレクションが生成されない問題とも無関係でいられる。
	//	=================================================================================

	/// @brief 宣言が全く重ならない2メソッドを持つ EntityLogic。
	/// @details インスタンス状態 (メンバ) を共有するため、宣言が重ならなくても
	///          この2つは直列化されなければならない。その規則の検証対象。
	class TwoMethodLogic final : public nox::EntityLogic<nox::test::updater_graph::TwoMethodLogic>
	{
	public:
		//	手書きの記述子はメソッドのアドレスを通常の文脈で取るので public に置く。
		void MethodA(nox::test::updater_graph::LayerC& c) { c.value += 1.0f; ++call_count; }
		void MethodB(nox::test::updater_graph::LayerD& d) { d.value += 1.0f; ++call_count; }

		nox::int32 call_count = 0;
	};

	/// @brief 上とは別の型。宣言も重ならないので TwoMethodLogic のどのメソッドとも衝突しない。
	class OtherLogic final : public nox::EntityLogic<nox::test::updater_graph::OtherLogic>
	{
	public:
		void MethodC(const nox::test::updater_graph::LayerA& a, nox::test::updater_graph::LayerB& b)
		{
			b.value = a.value;
		}
	};
}

namespace nox
{
	template<>
	struct EntityLogicMethodTable<nox::test::updater_graph::TwoMethodLogic> final
	{
		static constexpr std::array<nox::EntityLogicMethodDescriptor, 2> k_methods{
			nox::MakeEntityLogicMethodDescriptor<
				&nox::test::updater_graph::TwoMethodLogic::MethodA, nox::SystemPhaseType::Update>("MethodA"),
			nox::MakeEntityLogicMethodDescriptor<
				&nox::test::updater_graph::TwoMethodLogic::MethodB, nox::SystemPhaseType::Update>("MethodB"),
		};

		[[nodiscard]] static constexpr std::span<const nox::EntityLogicMethodDescriptor> GetMethods()noexcept
		{
			return std::span<const nox::EntityLogicMethodDescriptor>(k_methods.data(), k_methods.size());
		}
	};

	template<>
	struct EntityLogicMethodTable<nox::test::updater_graph::OtherLogic> final
	{
		static constexpr std::array<nox::EntityLogicMethodDescriptor, 1> k_methods{
			nox::MakeEntityLogicMethodDescriptor<
				&nox::test::updater_graph::OtherLogic::MethodC, nox::SystemPhaseType::Update>("MethodC"),
		};

		[[nodiscard]] static constexpr std::span<const nox::EntityLogicMethodDescriptor> GetMethods()noexcept
		{
			return std::span<const nox::EntityLogicMethodDescriptor>(k_methods.data(), k_methods.size());
		}
	};
}

namespace
{
	using namespace nox::test::updater_graph;

	/// @brief ComponentData のマスクだけを持つアクセス宣言を作る。
	[[nodiscard]] nox::UpdaterNodeAccess MakeAccess(
		const nox::ComponentMask& read_write_mask,
		const nox::ComponentMask& write_mask)noexcept
	{
		nox::UpdaterNodeAccess access{};
		access.read_write_mask = read_write_mask;
		access.write_mask = write_mask;
		return access;
	}

	/// @brief Service だけを持つアクセス宣言を作る。
	[[nodiscard]] nox::UpdaterNodeAccess MakeServiceAccess(
		const std::span<const nox::ServiceAccess> service_accesses)noexcept
	{
		nox::UpdaterNodeAccess access{};
		access.service_accesses = service_accesses;
		return access;
	}

	/// @brief 修飾名の末尾の1要素が name と一致するか。
	/// @details 単なる ends_with だと "OtherServiceReadSystem" が "ServiceReadSystem" に
	///          引っかかる。名前空間の区切り (::) の直後から一致することを要求する。
	[[nodiscard]] bool MatchesUnqualifiedName(const std::string_view full_name, const std::string_view name)noexcept
	{
		if (full_name.ends_with(name) == false)
		{
			return false;
		}
		if (full_name.size() == name.size())
		{
			return true;
		}
		return full_name[full_name.size() - name.size() - 1u] == ':';
	}

	/// @brief グラフのノードを名前で引く。EntitySystem は型名、EntityLogic は「型名::メソッド名」。
	[[nodiscard]] nox::uint32 FindLayerIndexByName(
		const std::span<const nox::UpdaterNode> nodes,
		const std::string_view name)
	{
		for (const nox::UpdaterNode& node : nodes)
		{
			if (node.kind == nox::UpdaterNodeKind::EntitySystem)
			{
				if (MatchesUnqualifiedName(node.system->GetDescriptor().name, name))
				{
					return node.layer_index;
				}
				continue;
			}

			//	EntityLogic は "型名::メソッド名" の後半で引く。
			if (node.method->name == name)
			{
				return node.layer_index;
			}
		}
		return std::numeric_limits<nox::uint32>::max();
	}
}

//	=====================================================================================
//	1. 衝突判定の規則そのもの
//	=====================================================================================

///	@brief	同じ ComponentData へ write する2つは衝突する。
TEST(UpdaterGraphConflict, WriteWriteOnSameComponentConflicts)
{
	const nox::ComponentMask mask_a = nox::MakeComponentMask<LayerA>();
	const nox::UpdaterNodeAccess left = MakeAccess(mask_a, mask_a);
	const nox::UpdaterNodeAccess right = MakeAccess(mask_a, mask_a);

	EXPECT_TRUE(nox::ConflictsUpdaterNodeAccess(left, right));
	EXPECT_TRUE(nox::ConflictsUpdaterNodeAccess(right, left));
}

///	@brief	write と read が同じ ComponentData で当たれば衝突する (両向き)。
TEST(UpdaterGraphConflict, WriteReadOnSameComponentConflicts)
{
	const nox::ComponentMask mask_a = nox::MakeComponentMask<LayerA>();
	const nox::UpdaterNodeAccess writer = MakeAccess(mask_a, mask_a);
	const nox::UpdaterNodeAccess reader = MakeAccess(mask_a, nox::ComponentMask{});

	EXPECT_TRUE(nox::ConflictsUpdaterNodeAccess(writer, reader));
	EXPECT_TRUE(nox::ConflictsUpdaterNodeAccess(reader, writer));
}

///	@brief	read 同士は衝突しない。ここが並列度の源。
TEST(UpdaterGraphConflict, ReadReadDoesNotConflict)
{
	const nox::ComponentMask mask_a = nox::MakeComponentMask<LayerA>();
	const nox::UpdaterNodeAccess left = MakeAccess(mask_a, nox::ComponentMask{});
	const nox::UpdaterNodeAccess right = MakeAccess(mask_a, nox::ComponentMask{});

	EXPECT_FALSE(nox::ConflictsUpdaterNodeAccess(left, right));
	EXPECT_FALSE(nox::ConflictsUpdaterNodeAccess(right, left));
}

///	@brief	触る ComponentData が全く重ならなければ衝突しない。
TEST(UpdaterGraphConflict, DisjointComponentsDoNotConflict)
{
	const nox::ComponentMask mask_a = nox::MakeComponentMask<LayerA>();
	const nox::ComponentMask mask_b = nox::MakeComponentMask<LayerB>();
	const nox::UpdaterNodeAccess left = MakeAccess(mask_a, mask_a);
	const nox::UpdaterNodeAccess right = MakeAccess(mask_b, mask_b);

	EXPECT_FALSE(nox::ConflictsUpdaterNodeAccess(left, right));
	EXPECT_FALSE(nox::ConflictsUpdaterNodeAccess(right, left));
}

///	@brief	Service でも ComponentData と同じ規則が成立する。
TEST(UpdaterGraphConflict, ServiceAccessFollowsTheSameRule)
{
	const nox::reflection::Type& type_x = nox::reflection::Typeof<LayerServiceX>();
	const nox::reflection::Type& type_y = nox::reflection::Typeof<LayerServiceY>();

	const std::array<nox::ServiceAccess, 1> write_x{ nox::ServiceAccess{ .type = &type_x, .write = true } };
	const std::array<nox::ServiceAccess, 1> read_x{ nox::ServiceAccess{ .type = &type_x, .write = false } };
	const std::array<nox::ServiceAccess, 1> read_x2{ nox::ServiceAccess{ .type = &type_x, .write = false } };
	const std::array<nox::ServiceAccess, 1> write_y{ nox::ServiceAccess{ .type = &type_y, .write = true } };

	//	write 同士
	EXPECT_TRUE(nox::ConflictsUpdaterNodeAccess(MakeServiceAccess(write_x), MakeServiceAccess(write_x)));
	//	write と read (両向き)
	EXPECT_TRUE(nox::ConflictsUpdaterNodeAccess(MakeServiceAccess(write_x), MakeServiceAccess(read_x)));
	EXPECT_TRUE(nox::ConflictsUpdaterNodeAccess(MakeServiceAccess(read_x), MakeServiceAccess(write_x)));
	//	read 同士は衝突しない
	EXPECT_FALSE(nox::ConflictsUpdaterNodeAccess(MakeServiceAccess(read_x), MakeServiceAccess(read_x2)));
	//	別 Service なら write 同士でも衝突しない
	EXPECT_FALSE(nox::ConflictsUpdaterNodeAccess(MakeServiceAccess(write_x), MakeServiceAccess(write_y)));
}

///	@brief	インスタンス状態を共有するノード同士は、宣言が全く重ならなくても衝突する。
///	@details	updater_graph.cpp が最初に見る規則。EntityLogic のメソッドは
///				同一インスタンスのメンバを共有するので、依存解析だけでは安全にならない。
TEST(UpdaterGraphConflict, SharedInstanceStateAlwaysConflicts)
{
	const nox::ComponentMask mask_a = nox::MakeComponentMask<LayerA>();
	const nox::ComponentMask mask_b = nox::MakeComponentMask<LayerB>();

	nox::UpdaterNodeAccess left = MakeAccess(mask_a, mask_a);
	nox::UpdaterNodeAccess right = MakeAccess(mask_b, mask_b);

	//	group が無ければ衝突しない (対照)。
	ASSERT_FALSE(nox::ConflictsUpdaterNodeAccess(left, right));

	left.group_index = 7u;
	right.group_index = 7u;
	EXPECT_TRUE(nox::ConflictsUpdaterNodeAccess(left, right));

	//	group が違えば元どおり衝突しない。
	right.group_index = 8u;
	EXPECT_FALSE(nox::ConflictsUpdaterNodeAccess(left, right));

	//	無効値同士は「共有相手がいない」であって「同じグループ」ではない。
	left.group_index = nox::k_invalid_updater_group_index;
	right.group_index = nox::k_invalid_updater_group_index;
	EXPECT_FALSE(nox::ConflictsUpdaterNodeAccess(left, right));
}

//	=====================================================================================
//	2. レイヤリング
//	=====================================================================================

///	@brief	互いに独立なノードは全て同一レイヤーへ載る。
///	@details	これが成立しないと「レイヤー内に複数ノード」が起きず、
///				World::ExecuteUpdaterGraphPhase の並列ディスパッチは
///				nodes.size() <= 1 の分岐で永遠に直列へ落ちる。
TEST(UpdaterGraphLayering, IndependentNodesShareOneLayer)
{
	const nox::ComponentMask mask_a = nox::MakeComponentMask<LayerA>();
	const nox::ComponentMask mask_b = nox::MakeComponentMask<LayerB>();
	const nox::ComponentMask mask_c = nox::MakeComponentMask<LayerC>();

	const std::array<nox::UpdaterNodeAccess, 3> accesses{
		MakeAccess(mask_a, mask_a),
		MakeAccess(mask_b, mask_b),
		MakeAccess(mask_c, mask_c),
	};

	std::array<nox::uint32, 3> layers{};
	const nox::uint32 layer_count = nox::BuildUpdaterLayerIndices(accesses, layers);

	EXPECT_EQ(layer_count, 1u);
	EXPECT_EQ(layers[0], 0u);
	EXPECT_EQ(layers[1], 0u);
	EXPECT_EQ(layers[2], 0u);
}

///	@brief	同じ ComponentData へ write する3つは3レイヤーに分かれ、登録順を保つ。
TEST(UpdaterGraphLayering, WritersOnSameComponentSerializeInRegistrationOrder)
{
	const nox::ComponentMask mask_a = nox::MakeComponentMask<LayerA>();
	const std::array<nox::UpdaterNodeAccess, 3> accesses{
		MakeAccess(mask_a, mask_a),
		MakeAccess(mask_a, mask_a),
		MakeAccess(mask_a, mask_a),
	};

	std::array<nox::uint32, 3> layers{};
	const nox::uint32 layer_count = nox::BuildUpdaterLayerIndices(accesses, layers);

	EXPECT_EQ(layer_count, 3u);
	EXPECT_EQ(layers[0], 0u);
	EXPECT_EQ(layers[1], 1u);
	EXPECT_EQ(layers[2], 2u);
}

///	@brief	writer を挟むと、後続の reader 群はまとめて次のレイヤーへ落ちる。
///	@details	「read 同士は衝突しない」ので reader 2つは同一レイヤーへ載り、
///				writer との衝突ぶんだけ後ろへずれる。宣言の依存関係とレイヤー順の整合。
TEST(UpdaterGraphLayering, ReadersAfterAWriterShareTheNextLayer)
{
	const nox::ComponentMask mask_a = nox::MakeComponentMask<LayerA>();

	const std::array<nox::UpdaterNodeAccess, 3> accesses{
		MakeAccess(mask_a, mask_a),                  //	n0: write A
		MakeAccess(mask_a, nox::ComponentMask{}),    //	n1: read  A
		MakeAccess(mask_a, nox::ComponentMask{}),    //	n2: read  A
	};

	std::array<nox::uint32, 3> layers{};
	const nox::uint32 layer_count = nox::BuildUpdaterLayerIndices(accesses, layers);

	EXPECT_EQ(layer_count, 2u);
	EXPECT_EQ(layers[0], 0u);
	EXPECT_EQ(layers[1], 1u);
	EXPECT_EQ(layers[2], 1u);
}

///	@brief	レイヤー番号は「衝突する先行ノードの最大レイヤー + 1」になる (最長経路)。
TEST(UpdaterGraphLayering, LayerIsLongestPathFromConflictingPredecessors)
{
	const nox::ComponentMask mask_a = nox::MakeComponentMask<LayerA>();
	const nox::ComponentMask mask_b = nox::MakeComponentMask<LayerB>();
	nox::ComponentMask mask_ab = mask_a;
	mask_ab.Merge(mask_b);

	const std::array<nox::UpdaterNodeAccess, 4> accesses{
		MakeAccess(mask_a, mask_a),     //	n0: write A          -> layer 0
		MakeAccess(mask_a, mask_a),     //	n1: write A (n0と衝突) -> layer 1
		MakeAccess(mask_b, mask_b),     //	n2: write B (独立)    -> layer 0
		MakeAccess(mask_ab, mask_ab),   //	n3: write A,B         -> max(1,0)+1 = 2
	};

	std::array<nox::uint32, 4> layers{};
	const nox::uint32 layer_count = nox::BuildUpdaterLayerIndices(accesses, layers);

	EXPECT_EQ(layer_count, 3u);
	EXPECT_EQ(layers[0], 0u);
	EXPECT_EQ(layers[1], 1u);
	EXPECT_EQ(layers[2], 0u);
	EXPECT_EQ(layers[3], 2u);
}

///	@brief	空入力はレイヤー0本。
TEST(UpdaterGraphLayering, EmptyInputProducesNoLayer)
{
	std::array<nox::uint32, 1> layers{};
	EXPECT_EQ(nox::BuildUpdaterLayerIndices(std::span<const nox::UpdaterNodeAccess>(), layers), 0u);
}

//	=====================================================================================
//	3. 本番の構築経路 (nox::UpdaterGraph::Rebuild)
//	=====================================================================================

namespace
{
	//	nox::EntityLogicStorage は記述子を参照で保持するので、実体に静的記憶域が要る。
	//	生成コードも型ごとに名前付きの constexpr オブジェクトを1つ置いている
	//	(entity_type_*.g.cpp の k_entity_logic_type_descriptor_*)。同じ形にする。
	constexpr nox::EntityLogicTypeDescriptor k_two_method_logic_descriptor =
		nox::MakeEntityLogicTypeDescriptor<TwoMethodLogic>();
	constexpr nox::EntityLogicTypeDescriptor k_other_logic_descriptor =
		nox::MakeEntityLogicTypeDescriptor<OtherLogic>();

	/// @brief 上で定義した System / EntityLogic を実際に組み上げてグラフを作る一式。
	/// @details World は介さない。Rebuild は span を受け取るだけなので、
	///          インスタンスをここで持って渡せば本番と同じ経路が回る。
	class GraphFixture final
	{
	public:
		GraphFixture()
		{
			//	登録順 = systems の並び → storages の並び → メソッド表の並び。
			systems_.push_back(&write_a_);
			systems_.push_back(&write_b_);
			systems_.push_back(&read_a_);
			systems_.push_back(&read_a2_);
			systems_.push_back(&service_write_);
			systems_.push_back(&service_read_);
			systems_.push_back(&other_service_read_);

			storages_.push_back(&two_method_storage_);
			storages_.push_back(&other_logic_storage_);

			graph_.Rebuild(
				std::span<nox::EntitySystemBase* const>(systems_.data(), systems_.size()),
				std::span<nox::EntityLogicStorage* const>(storages_.data(), storages_.size()));
		}

		[[nodiscard]] const nox::UpdaterGraph& GetGraph()const noexcept { return graph_; }

	private:
		WriteASystem write_a_;
		WriteBSystem write_b_;
		ReadASystem read_a_;
		ReadA2System read_a2_;
		ServiceWriteSystem service_write_;
		ServiceReadSystem service_read_;
		OtherServiceReadSystem other_service_read_;

		nox::EntityLogicStorage two_method_storage_{ k_two_method_logic_descriptor };
		nox::EntityLogicStorage other_logic_storage_{ k_other_logic_descriptor };

		std::vector<nox::EntitySystemBase*> systems_;
		std::vector<nox::EntityLogicStorage*> storages_;
		nox::UpdaterGraph graph_;
	};
}

///	@brief	実型で組んだグラフでも、レイヤー内に複数ノードが立つ。
///	@details	これが本タスクの前提条件。1ノードしか立たないレイヤーばかりだと
///				World::ExecuteUpdaterGraphPhase の並列ディスパッチは一度も踏まれない。
TEST(UpdaterGraphRebuild, SomeLayerHoldsMoreThanOneNode)
{
	const GraphFixture fixture;
	const nox::UpdaterGraph& graph = fixture.GetGraph();

	const nox::uint32 layer_count = graph.GetLayerCount(nox::SystemPhaseType::Update);
	ASSERT_GT(layer_count, 0u);

	nox::uint32 max_nodes_in_layer = 0u;
	for (nox::uint32 layer_index = 0u; layer_index < layer_count; ++layer_index)
	{
		const std::span<const nox::UpdaterNode> layer_nodes =
			graph.GetLayerNodes(nox::SystemPhaseType::Update, layer_index);
		max_nodes_in_layer = std::max(max_nodes_in_layer, static_cast<nox::uint32>(layer_nodes.size()));
	}

	EXPECT_GE(max_nodes_in_layer, 2u);
}

///	@brief	ノードの総数とレイヤー分割の整合。
TEST(UpdaterGraphRebuild, LayerPartitionCoversEveryNodeExactlyOnce)
{
	const GraphFixture fixture;
	const nox::UpdaterGraph& graph = fixture.GetGraph();

	const std::span<const nox::UpdaterNode> nodes = graph.GetNodes(nox::SystemPhaseType::Update);
	//	EntitySystem 7本 + TwoMethodLogic 2メソッド + OtherLogic 1メソッド。
	ASSERT_EQ(nodes.size(), 10u);

	const nox::uint32 layer_count = graph.GetLayerCount(nox::SystemPhaseType::Update);
	size_t total = 0u;
	for (nox::uint32 layer_index = 0u; layer_index < layer_count; ++layer_index)
	{
		const std::span<const nox::UpdaterNode> layer_nodes =
			graph.GetLayerNodes(nox::SystemPhaseType::Update, layer_index);
		for (const nox::UpdaterNode& node : layer_nodes)
		{
			EXPECT_EQ(node.layer_index, layer_index);
		}
		total += layer_nodes.size();
	}
	EXPECT_EQ(total, nodes.size());
}

///	@brief	独立な EntitySystem 同士 (WriteA / WriteB) は同一レイヤーへ載る。
TEST(UpdaterGraphRebuild, IndependentSystemsShareALayer)
{
	const GraphFixture fixture;
	const std::span<const nox::UpdaterNode> nodes =
		fixture.GetGraph().GetNodes(nox::SystemPhaseType::Update);

	const nox::uint32 write_a = FindLayerIndexByName(nodes, "WriteASystem");
	const nox::uint32 write_b = FindLayerIndexByName(nodes, "WriteBSystem");
	ASSERT_NE(write_a, std::numeric_limits<nox::uint32>::max());
	ASSERT_NE(write_b, std::numeric_limits<nox::uint32>::max());

	EXPECT_EQ(write_a, 0u);
	EXPECT_EQ(write_b, 0u);
}

///	@brief	LayerA を read するだけの2つは同一レイヤーへ載り、writer より後ろへ回る。
TEST(UpdaterGraphRebuild, ReadersShareALayerBehindTheWriter)
{
	const GraphFixture fixture;
	const std::span<const nox::UpdaterNode> nodes =
		fixture.GetGraph().GetNodes(nox::SystemPhaseType::Update);

	const nox::uint32 write_a = FindLayerIndexByName(nodes, "WriteASystem");
	const nox::uint32 read_a = FindLayerIndexByName(nodes, "ReadASystem");
	const nox::uint32 read_a2 = FindLayerIndexByName(nodes, "ReadA2System");
	ASSERT_NE(read_a, std::numeric_limits<nox::uint32>::max());
	ASSERT_NE(read_a2, std::numeric_limits<nox::uint32>::max());

	//	read 同士は衝突しないので同一レイヤー。
	EXPECT_EQ(read_a, read_a2);
	//	write A とは衝突するので後ろ。
	EXPECT_GT(read_a, write_a);
}

///	@brief	同一 Service に write が絡めば別レイヤーへ分かれる。
///	@details	この3 System は ComponentData が互いにも他のノードとも一切重ならない
///				(LayerE / LayerF / LayerG は専用) ので、レイヤーの分かれ方は
///				Service の規則だけで決まる。
TEST(UpdaterGraphRebuild, ServiceWriteSeparatesLayers)
{
	const GraphFixture fixture;
	const std::span<const nox::UpdaterNode> nodes =
		fixture.GetGraph().GetNodes(nox::SystemPhaseType::Update);

	const nox::uint32 service_write = FindLayerIndexByName(nodes, "ServiceWriteSystem");
	const nox::uint32 service_read = FindLayerIndexByName(nodes, "ServiceReadSystem");
	const nox::uint32 other_service_read = FindLayerIndexByName(nodes, "OtherServiceReadSystem");
	ASSERT_NE(service_write, std::numeric_limits<nox::uint32>::max());
	ASSERT_NE(service_read, std::numeric_limits<nox::uint32>::max());
	ASSERT_NE(other_service_read, std::numeric_limits<nox::uint32>::max());

	//	LayerServiceX を書く側は他の誰とも衝突しないので先頭レイヤー。
	EXPECT_EQ(service_write, 0u);
	//	同じ Service を読む側は、write と衝突するので1つ後ろへ回る。
	EXPECT_EQ(service_read, 1u);
	//	別 Service (LayerServiceY) しか触らない方は巻き込まれず先頭レイヤーのまま。
	EXPECT_EQ(other_service_read, 0u);
}

///	@brief	同一 EntityLogic 型の別メソッド同士は、宣言が重ならなくても別レイヤーへ分かれる。
///	@details	TwoMethodLogic::MethodA は LayerC、MethodB は LayerD しか宣言していない。
///				ComponentData も Service も一切重ならないので、依存解析だけなら
///				同一レイヤーへ載ってしまう。インスタンス状態の共有規則が効いていることの検証。
TEST(UpdaterGraphRebuild, MethodsOfTheSameEntityLogicAreSerialized)
{
	const GraphFixture fixture;
	const std::span<const nox::UpdaterNode> nodes =
		fixture.GetGraph().GetNodes(nox::SystemPhaseType::Update);

	const nox::uint32 method_a = FindLayerIndexByName(nodes, "MethodA");
	const nox::uint32 method_b = FindLayerIndexByName(nodes, "MethodB");
	ASSERT_NE(method_a, std::numeric_limits<nox::uint32>::max());
	ASSERT_NE(method_b, std::numeric_limits<nox::uint32>::max());

	//	登録順は MethodA -> MethodB。よって MethodB が後ろ。
	EXPECT_GT(method_b, method_a);

	//	宣言そのものは本当に重なっていないことを、同時に押さえておく
	//	(重なっていたら、このテストは共有規則ではなく依存解析を見ていることになる)。
	nox::ComponentMask rw_a{};
	nox::ComponentMask rw_b{};
	for (const nox::UpdaterNode& node : nodes)
	{
		if (node.kind != nox::UpdaterNodeKind::EntityLogicMethod) { continue; }
		if (node.method->name == "MethodA") { rw_a = node.access.read_write_mask; }
		if (node.method->name == "MethodB") { rw_b = node.access.read_write_mask; }
	}
	EXPECT_FALSE(rw_a.Intersects(rw_b));
}

///	@brief	別の EntityLogic 型のメソッドは、宣言が重ならなければ巻き込まれない。
TEST(UpdaterGraphRebuild, MethodsOfDifferentEntityLogicsAreIndependent)
{
	const GraphFixture fixture;
	const std::span<const nox::UpdaterNode> nodes =
		fixture.GetGraph().GetNodes(nox::SystemPhaseType::Update);

	const nox::uint32 method_a = FindLayerIndexByName(nodes, "MethodA");
	const nox::uint32 method_c = FindLayerIndexByName(nodes, "MethodC");
	ASSERT_NE(method_a, std::numeric_limits<nox::uint32>::max());
	ASSERT_NE(method_c, std::numeric_limits<nox::uint32>::max());

	//	MethodC は LayerA(read) / LayerB(write)。MethodA は LayerC(write)。重ならない。
	//	ただし MethodC は WriteBSystem(LayerB write) / OtherLogic とは別型なので、
	//	LayerB の writer とだけ直列化される。MethodA とは無関係でいられる。
	nox::ComponentMask rw_a{};
	nox::ComponentMask rw_c{};
	for (const nox::UpdaterNode& node : nodes)
	{
		if (node.kind != nox::UpdaterNodeKind::EntityLogicMethod) { continue; }
		if (node.method->name == "MethodA") { rw_a = node.access.read_write_mask; }
		if (node.method->name == "MethodC") { rw_c = node.access.read_write_mask; }
	}
	ASSERT_FALSE(rw_a.Intersects(rw_c));
	EXPECT_FALSE(nox::ConflictsUpdaterNodeAccess(
		nox::UpdaterNodeAccess{ .read_write_mask = rw_a, .write_mask = rw_a, .service_accesses = {}, .group_index = 0u },
		nox::UpdaterNodeAccess{ .read_write_mask = rw_c, .write_mask = rw_c, .service_accesses = {}, .group_index = 1u }));
}

//	=====================================================================================
//	4. 不変条件の全数検査 (ミューテーションを捕まえる本体)
//	=====================================================================================

///	@brief	同一レイヤーの任意のノード対は衝突しない。
///	@details	これが並列ディスパッチの安全性そのもの。
///				レイヤー内のノードはそのままワーカーへ配られるので、
///				1組でも衝突対が混ざっていればデータ競合になる。
TEST(UpdaterGraphInvariant, NoTwoNodesInTheSameLayerConflict)
{
	const GraphFixture fixture;
	const nox::UpdaterGraph& graph = fixture.GetGraph();

	const nox::uint32 layer_count = graph.GetLayerCount(nox::SystemPhaseType::Update);
	ASSERT_GT(layer_count, 0u);

	for (nox::uint32 layer_index = 0u; layer_index < layer_count; ++layer_index)
	{
		const std::span<const nox::UpdaterNode> layer_nodes =
			graph.GetLayerNodes(nox::SystemPhaseType::Update, layer_index);
		for (size_t i = 0u; i < layer_nodes.size(); ++i)
		{
			for (size_t j = i + 1u; j < layer_nodes.size(); ++j)
			{
				EXPECT_FALSE(nox::ConflictsUpdaterNodeAccess(layer_nodes[i].access, layer_nodes[j].access))
					<< "layer " << layer_index << " のノード " << layer_nodes[i].order_index
					<< " と " << layer_nodes[j].order_index << " が衝突している";
			}
		}
	}
}

///	@brief	衝突するノード対は、登録順の小さい方が必ず前のレイヤーに来る。
///	@details	レイヤー順が宣言の依存関係と整合していること。
///				等号を許さない (真に増加する) ので、衝突対が同居することもない。
TEST(UpdaterGraphInvariant, ConflictingPairsAreStrictlyOrderedByLayer)
{
	const GraphFixture fixture;
	const std::span<const nox::UpdaterNode> nodes =
		fixture.GetGraph().GetNodes(nox::SystemPhaseType::Update);
	ASSERT_FALSE(nodes.empty());

	nox::uint32 conflict_pair_count = 0u;
	for (const nox::UpdaterNode& from : nodes)
	{
		for (const nox::UpdaterNode& to : nodes)
		{
			if (from.order_index >= to.order_index) { continue; }
			if (nox::ConflictsUpdaterNodeAccess(from.access, to.access) == false) { continue; }

			++conflict_pair_count;
			EXPECT_LT(from.layer_index, to.layer_index)
				<< "衝突するノード " << from.order_index << " -> " << to.order_index
				<< " のレイヤー順が保たれていない";
		}
	}

	//	衝突辺が1本も無いグラフだと上のループが空回りするので、
	//	検査対象が実在することを押さえておく。
	EXPECT_GT(conflict_pair_count, 0u);
}

///	@brief	レイヤー番号は詰まっている (空のレイヤーが無い)。
TEST(UpdaterGraphInvariant, EveryLayerIsNonEmpty)
{
	const GraphFixture fixture;
	const nox::UpdaterGraph& graph = fixture.GetGraph();

	const nox::uint32 layer_count = graph.GetLayerCount(nox::SystemPhaseType::Update);
	for (nox::uint32 layer_index = 0u; layer_index < layer_count; ++layer_index)
	{
		EXPECT_FALSE(graph.GetLayerNodes(nox::SystemPhaseType::Update, layer_index).empty());
	}
}
