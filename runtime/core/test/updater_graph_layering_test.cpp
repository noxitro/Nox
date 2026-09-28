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
///				続いて 3 で組んだ実グラフに対して不変条件
///				  ・同一レイヤーの任意のノード対は衝突しない
///				  ・衝突するノード対は必ずレイヤーが真に増加する (全順序が保たれる)
///				を全数検査する。ここが「たまたま通っている」を排除する本体で、
///				衝突判定を壊すと必ず落ちる。
///
///				最後に全順序 (nox::UpdaterGraph::Rebuild / TryRebuild) を検証する。
///				明示辺 (RunAfter / RunBefore) は衝突が無くても直列化し、
///				順序が決まらない箇所は型名順で決まる (登録順を入れ替えても同じグラフになる)。
///				名前が解決できない・循環する宣言は構築失敗になる。
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
	//	core_test 専用の型を core/test/test_types.h へ足さずに済み、
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

	//	=================================================================================
	//	明示的な順序宣言 (RunAfter / RunBefore) の検証用
	//	どの型も専用の ComponentData しか触らないので、互いに衝突辺は1本も立たない。
	//	レイヤーが分かれるなら、それは明示辺だけによる。
	//	=================================================================================

	struct OrderH : nox::IComponentData { nox::float32 value; };
	struct OrderI : nox::IComponentData { nox::float32 value; };
	struct OrderJ : nox::IComponentData { nox::float32 value; };
	struct OrderK : nox::IComponentData { nox::float32 value; };
	struct OrderL : nox::IComponentData { nox::float32 value; };

	/// @brief 型名順では最後。OrderMidSystem の RunBefore で後ろへ、OrderAlphaSystem の RunAfter で前へ縛られる。
	/// @details nox::EntityCommands& を受けるので、コマンドバッファ番号が全順序で振られることの検証にも使う。
	class OrderZetaSystem final : public nox::EntitySystem<nox::test::updater_graph::OrderZetaSystem>
	{
	public:
		void OnUpdate(nox::test::updater_graph::OrderH& h, nox::EntityCommands&) { h.value += 1.0f; }
	};

	/// @brief 型名順では先頭だが、RunAfter で OrderZetaSystem の後ろへ回る。
	class OrderAlphaSystem final : public nox::EntitySystem<nox::test::updater_graph::OrderAlphaSystem>
	{
	public:
		using RunAfter = nox::TypeList<nox::test::updater_graph::OrderZetaSystem>;

		void OnUpdate(nox::test::updater_graph::OrderI& i, nox::EntityCommands&) { i.value += 1.0f; }
	};

	/// @brief RunBefore で OrderZetaSystem より前に置く。
	class OrderMidSystem final : public nox::EntitySystem<nox::test::updater_graph::OrderMidSystem>
	{
	public:
		using RunBefore = nox::TypeList<nox::test::updater_graph::OrderZetaSystem>;

		void OnUpdate(nox::test::updater_graph::OrderJ& j) { j.value += 1.0f; }
	};

	/// @brief EntityLogic 型に書いた RunAfter は、その型の更新メソッド全てに掛かる。
	class OrderLogic final : public nox::EntityLogic<nox::test::updater_graph::OrderLogic>
	{
	public:
		using RunAfter = nox::TypeList<nox::test::updater_graph::OrderAlphaSystem>;

		void Step(nox::test::updater_graph::OrderK& k) { k.value += 1.0f; }
	};

	/// @brief EntityLogic 型も RunAfter の相手に並べられる。
	class OrderTailSystem final : public nox::EntitySystem<nox::test::updater_graph::OrderTailSystem>
	{
	public:
		using RunAfter = nox::TypeList<nox::test::updater_graph::OrderLogic>;

		void OnUpdate(nox::test::updater_graph::OrderL& l) { l.value += 1.0f; }
	};

	//	---------------------------------------------------------------------------------
	//	構築失敗の検証用。いずれも TryRebuild で失敗が返ることを見る (Rebuild は abort する)。
	//	---------------------------------------------------------------------------------

	class CycleYSystem;

	/// @brief CycleYSystem と互いに RunAfter し合う (閉路)。
	class CycleXSystem final : public nox::EntitySystem<nox::test::updater_graph::CycleXSystem>
	{
	public:
		using RunAfter = nox::TypeList<nox::test::updater_graph::CycleYSystem>;

		void OnUpdate(nox::test::updater_graph::OrderH& h) { h.value += 1.0f; }
	};

	class CycleYSystem final : public nox::EntitySystem<nox::test::updater_graph::CycleYSystem>
	{
	public:
		using RunAfter = nox::TypeList<nox::test::updater_graph::CycleXSystem>;

		void OnUpdate(nox::test::updater_graph::OrderI& i) { i.value += 1.0f; }
	};

	/// @brief 閉路の下流にあるだけで、閉路には乗っていない型。
	/// @details 型名順では閉路の2つより前 ("CycleAfterX" < "CycleX") なので、
	///          報告が閉路の上のノードを指すこと(下流を指さないこと)の検証になる。
	class CycleAfterXSystem final : public nox::EntitySystem<nox::test::updater_graph::CycleAfterXSystem>
	{
	public:
		using RunAfter = nox::TypeList<nox::test::updater_graph::CycleXSystem>;

		void OnUpdate(nox::test::updater_graph::OrderJ& j) { j.value += 1.0f; }
	};

	/// @brief 自分自身を RunAfter に並べる (長さ1の閉路)。
	class SelfOrderSystem final : public nox::EntitySystem<nox::test::updater_graph::SelfOrderSystem>
	{
	public:
		using RunAfter = nox::TypeList<nox::test::updater_graph::SelfOrderSystem>;

		void OnUpdate(nox::test::updater_graph::OrderK& k) { k.value += 1.0f; }
	};

	/// @brief 前方宣言だけで、どこにも登録されない型。RunAfter に並べるのは名前だけなので定義は要らない。
	class NeverDefinedSystem;

	/// @brief 登録されていない型を RunAfter に並べる。
	class DanglingOrderSystem final : public nox::EntitySystem<nox::test::updater_graph::DanglingOrderSystem>
	{
	public:
		using RunAfter = nox::TypeList<nox::test::updater_graph::NeverDefinedSystem>;

		void OnUpdate(nox::test::updater_graph::OrderL& l) { l.value += 1.0f; }
	};

	/// @brief Init フェーズにしかノードを持たない型。
	class InitOnlySystem final
		: public nox::EntitySystem<nox::test::updater_graph::InitOnlySystem, nox::SystemPhaseType::Init>
	{
	public:
		void OnUpdate(nox::test::updater_graph::OrderH& h) { h.value += 1.0f; }
	};

	/// @brief Update フェーズから、Init にしかいない型を RunAfter に並べる (辺が1本も張れない)。
	class CrossPhaseSystem final : public nox::EntitySystem<nox::test::updater_graph::CrossPhaseSystem>
	{
	public:
		using RunAfter = nox::TypeList<nox::test::updater_graph::InitOnlySystem>;

		void OnUpdate(nox::test::updater_graph::OrderI& i) { i.value += 1.0f; }
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

	template<>
	struct EntityLogicMethodTable<nox::test::updater_graph::OrderLogic> final
	{
		static constexpr std::array<nox::EntityLogicMethodDescriptor, 1> k_methods{
			nox::MakeEntityLogicMethodDescriptor<
				&nox::test::updater_graph::OrderLogic::Step, nox::SystemPhaseType::Update>("Step"),
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

///	@brief	同じ ComponentData へ write する3つは3レイヤーに分かれ、渡した並びを保つ。
///	@details	BuildUpdaterLayerIndices が受け取る並びは、nox::UpdaterGraph が決めた全順序
///				(明示辺のトポロジカル順、決まらない箇所は型名順)。登録順ではない。
TEST(UpdaterGraphLayering, WritersOnSameComponentSerializeInGivenOrder)
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

///	@brief	明示辺は衝突が無くても後ろのレイヤーへ送る。
///	@details	3つとも触る ComponentData が重ならないので、明示辺が無ければ全て layer 0。
TEST(UpdaterGraphLayering, OrderEdgesSerializeIndependentNodes)
{
	const nox::ComponentMask mask_a = nox::MakeComponentMask<LayerA>();
	const nox::ComponentMask mask_b = nox::MakeComponentMask<LayerB>();
	const nox::ComponentMask mask_c = nox::MakeComponentMask<LayerC>();

	const std::array<nox::UpdaterNodeAccess, 3> accesses{
		MakeAccess(mask_a, mask_a),
		MakeAccess(mask_b, mask_b),
		MakeAccess(mask_c, mask_c),
	};

	//	n0 -> n2 だけ。n1 は巻き込まれない。
	{
		const std::array<nox::UpdaterOrderEdge, 1> edges{ nox::UpdaterOrderEdge{ .from = 0u, .to = 2u } };
		std::array<nox::uint32, 3> layers{};
		const nox::uint32 layer_count = nox::BuildUpdaterLayerIndicesWithOrderEdges(accesses, edges, layers);

		EXPECT_EQ(layer_count, 2u);
		EXPECT_EQ(layers[0], 0u);
		EXPECT_EQ(layers[1], 0u);
		EXPECT_EQ(layers[2], 1u);
	}

	//	n0 -> n1 -> n2 の鎖 (to の昇順に並べて渡す)。
	{
		const std::array<nox::UpdaterOrderEdge, 2> edges{
			nox::UpdaterOrderEdge{ .from = 0u, .to = 1u },
			nox::UpdaterOrderEdge{ .from = 1u, .to = 2u },
		};
		std::array<nox::uint32, 3> layers{};
		const nox::uint32 layer_count = nox::BuildUpdaterLayerIndicesWithOrderEdges(accesses, edges, layers);

		EXPECT_EQ(layer_count, 3u);
		EXPECT_EQ(layers[0], 0u);
		EXPECT_EQ(layers[1], 1u);
		EXPECT_EQ(layers[2], 2u);
	}

	//	辺が空なら短縮形 (BuildUpdaterLayerIndices) と同じ。
	{
		std::array<nox::uint32, 3> layers{};
		EXPECT_EQ(nox::BuildUpdaterLayerIndicesWithOrderEdges(accesses, std::span<const nox::UpdaterOrderEdge>(), layers), 1u);
		EXPECT_EQ(layers[0], 0u);
		EXPECT_EQ(layers[1], 0u);
		EXPECT_EQ(layers[2], 0u);
	}
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
		/// @param reverse_registration 登録順を逆にして組む。結果は変わらないはず。
		explicit GraphFixture(const bool reverse_registration = false)
		{
			//	登録順は結果に影響しない (全順序は型名順。UpdaterGraphOrder.RegistrationOrderDoesNotChangeTheGraph を参照)。
			systems_.push_back(&write_a_);
			systems_.push_back(&write_b_);
			systems_.push_back(&read_a_);
			systems_.push_back(&read_a2_);
			systems_.push_back(&service_write_);
			systems_.push_back(&service_read_);
			systems_.push_back(&other_service_read_);

			storages_.push_back(&two_method_storage_);
			storages_.push_back(&other_logic_storage_);

			if (reverse_registration)
			{
				std::ranges::reverse(systems_);
				std::ranges::reverse(storages_);
			}

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
///	@details	旧規則 (登録順) では両方ともレイヤー0だった。新規則では全順序が型名順なので、
///				型名で先に来る OtherLogic::MethodC (LayerA read / LayerB write) が
///				WriteA (LayerA write) とも WriteB (LayerB write) とも衝突し、2つとも1つ後ろへ回る。
///				独立な2つが同じレイヤーに載ること自体は変わらない。
TEST(UpdaterGraphRebuild, IndependentSystemsShareALayer)
{
	const GraphFixture fixture;
	const std::span<const nox::UpdaterNode> nodes =
		fixture.GetGraph().GetNodes(nox::SystemPhaseType::Update);

	const nox::uint32 write_a = FindLayerIndexByName(nodes, "WriteASystem");
	const nox::uint32 write_b = FindLayerIndexByName(nodes, "WriteBSystem");
	ASSERT_NE(write_a, std::numeric_limits<nox::uint32>::max());
	ASSERT_NE(write_b, std::numeric_limits<nox::uint32>::max());

	EXPECT_EQ(write_a, write_b);
	EXPECT_EQ(write_a, 1u);
	EXPECT_EQ(FindLayerIndexByName(nodes, "MethodC"), 0u);
}

///	@brief	LayerA を read するだけの2つは同一レイヤーへ載り、writer とは直列化される。
///	@details	旧規則 (登録順) では writer (WriteASystem) が先だった。新規則では明示辺が無いので
///				型名順 ("ReadA2System" < "ReadASystem" < "WriteASystem") で決まり、writer が後ろへ回る。
///				writer を先にしたければ reader 側に RunAfter を書く (UpdaterGraphOrder.* を参照)。
TEST(UpdaterGraphRebuild, ReadersShareALayerAndTheWriterFollowsByTypeName)
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
	EXPECT_EQ(read_a, 0u);
	//	write A とは衝突するので別レイヤー。型名順で writer が後ろ。
	EXPECT_GT(write_a, read_a);
}

///	@brief	同一 Service に write が絡めば別レイヤーへ分かれる。
///	@details	この3 System は ComponentData が互いにも他のノードとも一切重ならない
///				(LayerE / LayerF / LayerG は専用) ので、レイヤーの分かれ方は
///				Service の規則だけで決まる。
///				旧規則 (登録順) では ServiceWrite が先だった。新規則では型名順
///				("ServiceReadSystem" < "ServiceWriteSystem") で読む側が先になる。
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

	//	LayerServiceX を読む側は型名順で先なので先頭レイヤー。
	EXPECT_EQ(service_read, 0u);
	//	同じ Service を書く側は、read と衝突するので1つ後ろへ回る。
	EXPECT_EQ(service_write, 1u);
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

	//	同じ型のメソッド同士はメソッド名順 (MethodA -> MethodB)。よって MethodB が後ろ。
	//	旧規則ではメソッド表の並び (登録順) だったが、この型ではどちらでも同じ並びになる。
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

///	@brief	衝突するノード対は、全順序 (order_index) の小さい方が必ず前のレイヤーに来る。
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

//	=====================================================================================
//	5. 全順序 (明示辺 + 型名順)
//	=====================================================================================

namespace
{
	constexpr nox::EntityLogicTypeDescriptor k_order_logic_descriptor =
		nox::MakeEntityLogicTypeDescriptor<OrderLogic>();

	/// @brief 明示辺だけで直列化される5ノードのグラフ。
	/// @details 期待する全順序は OrderMid -> OrderZeta -> OrderAlpha -> OrderLogic::Step -> OrderTail。
	///          型名順 (Alpha < Logic < Mid < Tail < Zeta) とはほぼ逆で、明示辺が型名順に勝つことを見る。
	class OrderFixture final
	{
	public:
		/// @param reverse_registration 登録順を逆にして組む。結果は変わらないはず。
		explicit OrderFixture(const bool reverse_registration = false)
		{
			//	登録順はわざと型名順とも期待する全順序ともずらしてある。
			systems_.push_back(&zeta_);
			systems_.push_back(&tail_);
			systems_.push_back(&alpha_);
			systems_.push_back(&mid_);
			storages_.push_back(&logic_storage_);

			if (reverse_registration)
			{
				std::ranges::reverse(systems_);
				std::ranges::reverse(storages_);
			}

			graph_.Rebuild(
				std::span<nox::EntitySystemBase* const>(systems_.data(), systems_.size()),
				std::span<nox::EntityLogicStorage* const>(storages_.data(), storages_.size()));
		}

		[[nodiscard]] const nox::UpdaterGraph& GetGraph()const noexcept { return graph_; }

	private:
		OrderZetaSystem zeta_;
		OrderAlphaSystem alpha_;
		OrderMidSystem mid_;
		OrderTailSystem tail_;
		nox::EntityLogicStorage logic_storage_{ k_order_logic_descriptor };

		std::vector<nox::EntitySystemBase*> systems_;
		std::vector<nox::EntityLogicStorage*> storages_;
		nox::UpdaterGraph graph_;
	};

	/// @brief ノードの型名。EntitySystem は System の型、EntityLogic は Logic の型。
	[[nodiscard]] std::string_view GetNodeTypeName(const nox::UpdaterNode& node)noexcept
	{
		return (node.kind == nox::UpdaterNodeKind::EntitySystem)
			? node.system->GetDescriptor().name
			: node.storage->GetDescriptor().name;
	}

	/// @brief ノードのメソッド名。EntitySystem は空。
	[[nodiscard]] std::string_view GetNodeMethodName(const nox::UpdaterNode& node)noexcept
	{
		return (node.kind == nox::UpdaterNodeKind::EntitySystem) ? std::string_view() : node.method->name;
	}

	/// @brief order_index の順に並べたノード。GetNodes は (レイヤー, 全順序) 順なので並べ直す。
	[[nodiscard]] std::vector<const nox::UpdaterNode*> SortByOrderIndex(const std::span<const nox::UpdaterNode> nodes)
	{
		std::vector<const nox::UpdaterNode*> sorted(nodes.size(), nullptr);
		for (const nox::UpdaterNode& node : nodes)
		{
			if (node.order_index < sorted.size())
			{
				sorted[node.order_index] = &node;
			}
		}
		return sorted;
	}

	/// @brief 2つのグラフのフェーズ内ノード列が、インスタンスの違いを除いて一致するか。
	/// @details インスタンスのアドレスと group_index (storages の並びでの番号) は登録順で変わるので比べない。
	///          型名・メソッド名・レイヤー・全順序・コマンドバッファ番号・宣言を比べる。
	void ExpectSameGraph(
		const nox::UpdaterGraph& expected,
		const nox::UpdaterGraph& actual,
		const nox::SystemPhaseType phase_type)
	{
		const std::span<const nox::UpdaterNode> expected_nodes = expected.GetNodes(phase_type);
		const std::span<const nox::UpdaterNode> actual_nodes = actual.GetNodes(phase_type);
		ASSERT_EQ(expected_nodes.size(), actual_nodes.size());
		EXPECT_EQ(expected.GetLayerCount(phase_type), actual.GetLayerCount(phase_type));
		EXPECT_EQ(expected.GetCommandBufferCount(phase_type), actual.GetCommandBufferCount(phase_type));

		for (size_t index = 0u; index < expected_nodes.size(); ++index)
		{
			const nox::UpdaterNode& left = expected_nodes[index];
			const nox::UpdaterNode& right = actual_nodes[index];
			EXPECT_EQ(GetNodeTypeName(left), GetNodeTypeName(right)) << "index " << index;
			EXPECT_EQ(GetNodeMethodName(left), GetNodeMethodName(right)) << "index " << index;
			EXPECT_EQ(left.kind, right.kind) << "index " << index;
			EXPECT_EQ(left.layer_index, right.layer_index) << "index " << index;
			EXPECT_EQ(left.order_index, right.order_index) << "index " << index;
			EXPECT_EQ(left.command_buffer_index, right.command_buffer_index) << "index " << index;
			EXPECT_TRUE(left.access.read_write_mask == right.access.read_write_mask) << "index " << index;
			EXPECT_TRUE(left.access.write_mask == right.access.write_mask) << "index " << index;
		}
	}

	/// @brief System だけで TryRebuild を回し、失敗したらグラフが空になっていることまで確かめる。
	[[nodiscard]] nox::UpdaterGraphBuildResult TryBuildSystems(std::vector<nox::EntitySystemBase*> systems)
	{
		nox::UpdaterGraph graph;
		const nox::UpdaterGraphBuildResult result = graph.TryRebuild(
			std::span<nox::EntitySystemBase* const>(systems.data(), systems.size()),
			std::span<nox::EntityLogicStorage* const>());

		if (result.IsSuccess() == false)
		{
			//	半端なグラフで走らないよう、失敗したら全フェーズが空になる。
			for (nox::uint8 phase_index = 0u; phase_index < nox::util::ToUnderlying(nox::SystemPhaseType::_Max); ++phase_index)
			{
				const nox::SystemPhaseType phase_type = static_cast<nox::SystemPhaseType>(phase_index);
				EXPECT_TRUE(graph.GetNodes(phase_type).empty());
				EXPECT_EQ(graph.GetLayerCount(phase_type), 0u);
				EXPECT_EQ(graph.GetCommandBufferCount(phase_type), 0u);
			}
		}
		return result;
	}
}

///	@brief	明示辺 (RunAfter / RunBefore) があると、衝突が無くても後ろのレイヤーに置かれる。
///	@details	5ノードはどの組も衝突しない (先に全数で確かめる)。それでも 5 レイヤーに分かれ、
///				型名順ではなく明示辺の向きに並ぶ。EntityLogic の型に書いた RunAfter と、
///				EntityLogic の型を相手に並べた RunAfter の両方が効くことも含む。
TEST(UpdaterGraphOrder, ExplicitEdgesSerializeWithoutConflicts)
{
	const OrderFixture fixture;
	const nox::UpdaterGraph& graph = fixture.GetGraph();
	const std::span<const nox::UpdaterNode> nodes = graph.GetNodes(nox::SystemPhaseType::Update);
	ASSERT_EQ(nodes.size(), 5u);

	//	前提: 衝突辺は1本も無い。レイヤーが分かれるなら明示辺だけによる。
	for (size_t i = 0u; i < nodes.size(); ++i)
	{
		for (size_t j = i + 1u; j < nodes.size(); ++j)
		{
			ASSERT_FALSE(nox::ConflictsUpdaterNodeAccess(nodes[i].access, nodes[j].access));
		}
	}

	EXPECT_EQ(graph.GetLayerCount(nox::SystemPhaseType::Update), 5u);

	const std::vector<const nox::UpdaterNode*> in_order = SortByOrderIndex(nodes);
	ASSERT_EQ(in_order.size(), 5u);
	const std::array<std::string_view, 5> expected_types{
		nox::util::GetTypeName<OrderMidSystem>(),
		nox::util::GetTypeName<OrderZetaSystem>(),
		nox::util::GetTypeName<OrderAlphaSystem>(),
		nox::util::GetTypeName<OrderLogic>(),
		nox::util::GetTypeName<OrderTailSystem>(),
	};
	for (nox::uint32 order_index = 0u; order_index < in_order.size(); ++order_index)
	{
		ASSERT_NE(in_order[order_index], nullptr);
		EXPECT_EQ(GetNodeTypeName(*in_order[order_index]), expected_types[order_index]) << "order " << order_index;
		//	鎖なので、全順序の位置がそのままレイヤー番号になる。
		EXPECT_EQ(in_order[order_index]->layer_index, order_index) << "order " << order_index;
	}
	EXPECT_EQ(GetNodeMethodName(*in_order[3]), "Step");
}

///	@brief	コマンドバッファ番号は、型名順ではなく全順序の昇順で振られる。
///	@details	nox::EntityCommands& を受けるのは OrderZeta と OrderAlpha。型名順なら Alpha が先だが、
///				明示辺で Zeta が先に実行されるので、再生 (バッファ番号順) も Zeta が先でなければならない。
TEST(UpdaterGraphOrder, CommandBufferIndicesFollowTheTotalOrder)
{
	const OrderFixture fixture;
	const nox::UpdaterGraph& graph = fixture.GetGraph();
	const std::span<const nox::UpdaterNode> nodes = graph.GetNodes(nox::SystemPhaseType::Update);

	EXPECT_EQ(graph.GetCommandBufferCount(nox::SystemPhaseType::Update), 2u);

	nox::uint32 zeta_buffer = nox::k_invalid_updater_command_buffer_index;
	nox::uint32 alpha_buffer = nox::k_invalid_updater_command_buffer_index;
	for (const nox::UpdaterNode& node : nodes)
	{
		if (GetNodeTypeName(node) == nox::util::GetTypeName<OrderZetaSystem>())
		{
			zeta_buffer = node.command_buffer_index;
		}
		else if (GetNodeTypeName(node) == nox::util::GetTypeName<OrderAlphaSystem>())
		{
			alpha_buffer = node.command_buffer_index;
		}
		else
		{
			//	nox::EntityCommands& を受けないノードにはバッファを割り当てない。
			EXPECT_EQ(node.command_buffer_index, nox::k_invalid_updater_command_buffer_index);
		}
	}

	EXPECT_EQ(zeta_buffer, 0u);
	EXPECT_EQ(alpha_buffer, 1u);
}

///	@brief	登録順を入れ替えても、レイヤーと直列化順 (全順序) が変わらない。
///	@details	旧規則では登録順 (= 生成器の走査順) がそのまま直列化順だった。
///				新規則では全順序が明示辺と型名だけで決まるので、渡す並びを逆にしても同じグラフになる。
///				衝突で直列化されるグラフ (GraphFixture) と、明示辺で直列化されるグラフ (OrderFixture) の両方で見る。
TEST(UpdaterGraphOrder, RegistrationOrderDoesNotChangeTheGraph)
{
	{
		const GraphFixture forward(false);
		const GraphFixture reversed(true);
		ExpectSameGraph(forward.GetGraph(), reversed.GetGraph(), nox::SystemPhaseType::Update);
	}
	{
		const OrderFixture forward(false);
		const OrderFixture reversed(true);
		ExpectSameGraph(forward.GetGraph(), reversed.GetGraph(), nox::SystemPhaseType::Update);
	}
}

///	@brief	明示辺が無い衝突は、型名順で直列化される。
///	@details	GraphFixture の全順序を先頭から並べると、(型名, メソッド名) の昇順になっているはず。
///				明示辺を1本も持たないグラフなので、全順序は型名順そのもの。
TEST(UpdaterGraphOrder, WithoutExplicitEdgesTheTotalOrderIsTypeNameOrder)
{
	const GraphFixture fixture;
	const std::vector<const nox::UpdaterNode*> in_order =
		SortByOrderIndex(fixture.GetGraph().GetNodes(nox::SystemPhaseType::Update));
	ASSERT_EQ(in_order.size(), 10u);

	for (size_t index = 1u; index < in_order.size(); ++index)
	{
		ASSERT_NE(in_order[index - 1u], nullptr);
		ASSERT_NE(in_order[index], nullptr);
		const std::string_view previous_type = GetNodeTypeName(*in_order[index - 1u]);
		const std::string_view current_type = GetNodeTypeName(*in_order[index]);
		const bool ascending =
			(previous_type < current_type) ||
			((previous_type == current_type) && (GetNodeMethodName(*in_order[index - 1u]) < GetNodeMethodName(*in_order[index])));
		EXPECT_TRUE(ascending) << previous_type << " / " << current_type;
	}
}

///	@brief	明示辺が循環していれば構築失敗 (Rebuild なら abort する経路)。
///	@details	CycleX と CycleY は互いに RunAfter し合う。CycleAfterX は閉路の下流にいるだけで、
///				型名順では閉路の2つより前に来る。報告は閉路の上の辺を指し、下流を指さないこと。
TEST(UpdaterGraphOrder, CycleOfExplicitEdgesFailsToBuild)
{
	CycleXSystem cycle_x;
	CycleYSystem cycle_y;
	CycleAfterXSystem after_x;

	const nox::UpdaterGraphBuildResult result = TryBuildSystems({ &after_x, &cycle_x, &cycle_y });
	EXPECT_EQ(result.error, nox::UpdaterGraphBuildError::OrderCycle);
	EXPECT_EQ(result.phase, nox::SystemPhaseType::Update);

	const std::string_view name_x = nox::util::GetTypeName<CycleXSystem>();
	const std::string_view name_y = nox::util::GetTypeName<CycleYSystem>();
	EXPECT_TRUE((result.declaring_type_name == name_x) || (result.declaring_type_name == name_y))
		<< result.declaring_type_name;
	EXPECT_TRUE((result.target_type_name == name_x) || (result.target_type_name == name_y))
		<< result.target_type_name;
	EXPECT_NE(result.declaring_type_name, result.target_type_name);
}

///	@brief	自分自身を RunAfter に並べるのも循環 (長さ1)。
TEST(UpdaterGraphOrder, SelfReferenceFailsToBuild)
{
	SelfOrderSystem self_order;

	const nox::UpdaterGraphBuildResult result = TryBuildSystems({ &self_order });
	EXPECT_EQ(result.error, nox::UpdaterGraphBuildError::OrderCycle);
	EXPECT_EQ(result.declaring_type_name, nox::util::GetTypeName<SelfOrderSystem>());
	EXPECT_EQ(result.target_type_name, nox::util::GetTypeName<SelfOrderSystem>());
}

///	@brief	登録されていない型への明示辺は構築失敗 (黙って無視しない)。
TEST(UpdaterGraphOrder, UnresolvedNameFailsToBuild)
{
	DanglingOrderSystem dangling;
	//	明示辺を持たない型を混ぜておく(こちらが巻き添えで失敗扱いにならないこと)。
	WriteASystem unrelated;

	const nox::UpdaterGraphBuildResult result = TryBuildSystems({ &unrelated, &dangling });
	EXPECT_EQ(result.error, nox::UpdaterGraphBuildError::UnresolvedOrderTarget);
	EXPECT_EQ(result.declaring_type_name, nox::util::GetTypeName<DanglingOrderSystem>());
	EXPECT_EQ(result.target_type_name, nox::util::GetTypeName<NeverDefinedSystem>());
}

///	@brief	RunBefore の相手が登録されていなくても同じく構築失敗。
///	@details	OrderMidSystem は RunBefore<OrderZetaSystem> を宣言している。Zeta を渡さなければ解決できない。
TEST(UpdaterGraphOrder, UnresolvedRunBeforeFailsToBuild)
{
	OrderMidSystem mid;

	const nox::UpdaterGraphBuildResult result = TryBuildSystems({ &mid });
	EXPECT_EQ(result.error, nox::UpdaterGraphBuildError::UnresolvedOrderTarget);
	EXPECT_EQ(result.declaring_type_name, nox::util::GetTypeName<OrderMidSystem>());
	EXPECT_EQ(result.target_type_name, nox::util::GetTypeName<OrderZetaSystem>());
}

///	@brief	登録されていても、同じフェーズにノードを持たない型への明示辺は構築失敗。
///	@details	辺が1本も張れないので、放っておくと宣言が黙って無効になる。
TEST(UpdaterGraphOrder, TargetInAnotherPhaseFailsToBuild)
{
	InitOnlySystem init_only;
	CrossPhaseSystem cross_phase;

	const nox::UpdaterGraphBuildResult result = TryBuildSystems({ &init_only, &cross_phase });
	EXPECT_EQ(result.error, nox::UpdaterGraphBuildError::OrderTargetInOtherPhase);
	EXPECT_EQ(result.declaring_type_name, nox::util::GetTypeName<CrossPhaseSystem>());
	EXPECT_EQ(result.target_type_name, nox::util::GetTypeName<InitOnlySystem>());
}
