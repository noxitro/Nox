// Copyright (C) 2026 NOX ENGINE All rights reserved.

/// @file	entity_system.h
/// @brief	大量のentityを一括処理するSystem。
/// @details 宣言は OnUpdate の引数リストだけ。const参照が読み取り、非const参照が書き込みを意味し、
///          その宣言がそのままUpdaterGraphの依存解析の入力になる。宣言外のComponentDataには
///          そもそも触れられないため、宣言と実装が乖離しない。
///
///          仮想関数は使わない。実行はTDerived::OnUpdateへ直接束縛された静的サンクを経由するため、
///          ForEachのループ内に間接呼び出しが1つも残らない。
///
///          購読はリフレクション生成コードが行う。ヘッダにクラスを定義するだけで
///          nox::GetEntitySystemTypes() の表に載り、Worldが自動生成・自動実行する。
#pragma once
#include	"entity_query.h"
#include	"system_phase_type.h"

namespace nox
{
	class World;
	class EntitySystemBase;

	/// @brief EntitySystem型ごとに1つだけ作られる静的記述子。
	/// @details 全メンバが定数式で埋まるため定数初期化される。ヒープもコンテナも動的初期化も使わない。
	struct EntitySystemTypeDescriptor final
	{
		/// @brief インスタンス生成。World::Init時に1回だけ呼ばれる。
		nox::EntitySystemBase* (*create)();
		/// @brief インスタンス破棄。仮想デストラクタの代わり。
		void (*destroy)(nox::EntitySystemBase*)noexcept;
		/// @brief 読み書きするComponentDataのマスク。Queryの必須条件を兼ねる。
		nox::ComponentMask(*make_read_write_mask)()noexcept;
		/// @brief 書き込みするComponentDataのマスク。依存解析に使う。
		nox::ComponentMask(*make_write_mask)()noexcept;
		/// @brief 読み書きするServiceの一覧。依存解析に使う。確保は走らない。
		std::span<const nox::ServiceAccess>(*get_service_accesses)()noexcept;
		/// @brief 実行本体。TDerived::OnUpdateへ静的に束縛されている。
		void (*execute)(nox::EntitySystemBase&, nox::World&);
		/// @brief Chunk1つ分だけの実行本体。executeと同じくTDerived::OnUpdateへ静的に束縛されている。
		/// @details parallel_for_eachがtrueのとき、Worldがこれをジョブとしてワーカーへ配る。
		void (*execute_chunk)(nox::EntitySystemBase&, nox::World&, nox::Archetype&, nox::uint32 chunk_index);
		std::string_view name;
		nox::SystemPhaseType phase;
		/// @brief OnUpdateをChunk単位で並列実行してよいか。 nox::IsParallelForEachEntitySystem を参照。
		bool parallel_for_each;
		/// @brief このSystemが遅延構造変更を出しうるか(= nox::EntityCommands& を宣言しているか)。
		/// @details 引数リストから導出される。宣言していないSystemは1コマンドも積めないので、
		///          Worldはこのノードのぶんのコマンドバッファを確保しない。
		bool emits_structural_change;
	};

	/// @brief EntitySystemの非テンプレート基底。Worldはこの型でのみ保持する。
	/// @details 仮想関数を持たない。多態は記述子の関数ポインタで行う。
	class EntitySystemBase
	{
	public:
		EntitySystemBase(const EntitySystemBase&) = delete;
		EntitySystemBase& operator=(const EntitySystemBase&) = delete;

		inline void Execute(nox::World& world) { descriptor_.execute(*this, world); }

		/// @brief Chunk1つ分だけ実行する。Chunk並列実行の1ジョブ分。
		inline void ExecuteChunk(nox::World& world, nox::Archetype& archetype, const nox::uint32 chunk_index)
		{
			descriptor_.execute_chunk(*this, world, archetype, chunk_index);
		}

		/// @brief 仮想デストラクタの代わり。記述子が具象型を知っている。
		inline void Destroy()noexcept { descriptor_.destroy(this); }

		[[nodiscard]] inline const nox::EntitySystemTypeDescriptor& GetDescriptor()const noexcept { return descriptor_; }
		[[nodiscard]] inline nox::EntityQuery& GetQuery()noexcept { return query_; }
		[[nodiscard]] inline const nox::EntityQuery& GetQuery()const noexcept { return query_; }

	protected:
		inline explicit EntitySystemBase(const nox::EntitySystemTypeDescriptor& descriptor)noexcept :
			descriptor_(descriptor),
			query_()
		{
			query_.Reset(descriptor.make_read_write_mask());
		}

		//	Destroy()経由でのみ破棄されるため非virtual。
		inline ~EntitySystemBase() = default;

	private:
		const nox::EntitySystemTypeDescriptor& descriptor_;
		nox::EntityQuery query_;
	};

	namespace detail
	{
		/// @brief TSystem::OnUpdateへ静的に束縛された実行本体。仮想関数もstd::functionも介さない。
		template<class TSystem>
		void ExecuteEntitySystem(nox::EntitySystemBase& self, nox::World& world)
		{
			using Signature = typename TSystem::template SignatureOf<>;
			auto& derived = static_cast<TSystem&>(self);
			nox::detail::EntityInvokerOf<Signature>::ForEachEntity(world, self.GetQuery(), derived, &TSystem::OnUpdate);
		}

		/// @brief TSystem::OnUpdateへ静的に束縛された、Chunk1つ分の実行本体。
		template<class TSystem>
		void ExecuteEntitySystemChunk(
			nox::EntitySystemBase& self,
			nox::World& world,
			nox::Archetype& archetype,
			const nox::uint32 chunk_index)
		{
			using Signature = typename TSystem::template SignatureOf<>;
			auto& derived = static_cast<TSystem&>(self);
			nox::detail::EntityInvokerOf<Signature>::ForEachEntityInChunk(
				world, archetype, chunk_index, derived, &TSystem::OnUpdate);
		}
	}

	/// @brief TSystemが `static constexpr bool k_parallel_for_each` でChunk並列実行を宣言しているか。
	/// @details 宣言が無い型は既定でfalse。マクロも生成器も要らず、クラス定義を見れば分かる形にしてある。
	///
	///          trueにしてよい条件(System作者が満たすべき責務):
	///            - OnUpdateの結果が「entityを処理する順序」に依存しないこと。
	///            - 宣言したComponentData / Service 以外の、entity間で共有される状態に触れないこと。
	///              とくに **System自身のメンバへの書き込みは安全ではない**。Chunkごとのジョブは
	///              同一のSystemインスタンス上で同時に走るため、メンバのインクリメントも代入も競合する。
	///              例えば nox::test::ecs::TestMoveSystem の processed_count / last_entity は
	///              まさにこの種の状態であり、あのSystemはk_parallel_for_eachを宣言してはならない。
	///            - 同一Chunk内・Chunk間の他の行を覗かないこと(引数で渡された行だけを触ること)。
	///
	///          逆に安全なのは「宣言したComponentDataの、自分の行だけを読み書きする」形。
	///          Chunkは互いに素なメモリなので、この形なら2つのワーカーが同じバイトに触ることはない。
	///
	///          【nox::EntityCommands& と併記できない理由】
	///          遅延構造変更のコマンドバッファは「ノード1つにつき1本」であり、
	///          同一ノードのChunkジョブ全員がそこへ積む。つまりコマンドバッファは
	///          上記の「entity間で共有される状態」そのもので、積まれる順序は
	///          ワーカーのスケジュールで毎フレーム変わる。
	///          Playback順が変われば、たとえ競合コマンドが無くてもArchetypeへの
	///          行の挿入順が変わるため、次フレームの列挙順まで変わる。
	///          そこで宣言の矛盾としてコンパイル時に弾く
	///          (nox::MakeEntitySystemTypeDescriptor の static_assert)。
	///          Chunk単位のサブバッファを持たせれば両立できるが、
	///          Chunk数は実行時にしか決まらないため固定確保と噛み合わない。将来の課題として切り離す。
	template<class TSystem>
	[[nodiscard]] constexpr bool IsParallelForEachEntitySystem()noexcept
	{
		if constexpr (requires { { TSystem::k_parallel_for_each } -> std::convertible_to<bool>; })
		{
			return static_cast<bool>(TSystem::k_parallel_for_each);
		}
		else
		{
			return false;
		}
	}

	/// @brief EntitySystem型の記述子を作る。
	/// @details TSystem::k_phase と TSystem::SignatureOf<> だけを読む。CRTP基底には何も持たせない。
	template<class TSystem>
	[[nodiscard]] constexpr nox::EntitySystemTypeDescriptor MakeEntitySystemTypeDescriptor()noexcept
	{
		using Signature = typename TSystem::template SignatureOf<>;
		static_assert(nox::detail::ValidateEntityMethod<decltype(&TSystem::OnUpdate)>());
		//	Chunk並列の宣言と、遅延構造変更を出す宣言(nox::EntityCommands&)は両立しない。
		//	理由は nox::IsParallelForEachEntitySystem のコメントを参照。
		static_assert(
			nox::IsParallelForEachEntitySystem<TSystem>() == false ||
			Signature::k_commands_parameter_count == 0u,
			"k_parallel_for_each を宣言したSystemは nox::EntityCommands& を受け取れません"
			"(Chunkジョブ間でコマンドの順序が決まらないため)。"
			"構造変更を出すなら k_parallel_for_each を外してください");

		return nox::EntitySystemTypeDescriptor{
			.create = []() -> nox::EntitySystemBase* { return new TSystem(); },
			.destroy = [](nox::EntitySystemBase* instance)noexcept { delete static_cast<TSystem*>(instance); },
			.make_read_write_mask = []()noexcept { return Signature::GetReadWriteMask(); },
			.make_write_mask = []()noexcept { return Signature::GetWriteMask(); },
			.get_service_accesses = []()noexcept { return Signature::GetServiceAccesses(); },
			.execute = &nox::detail::ExecuteEntitySystem<TSystem>,
			.execute_chunk = &nox::detail::ExecuteEntitySystemChunk<TSystem>,
			.name = nox::util::GetTypeName<TSystem>(),
			.phase = TSystem::k_phase,
			.parallel_for_each = nox::IsParallelForEachEntitySystem<TSystem>(),
			.emits_structural_change = (Signature::k_commands_parameter_count != 0u),
		};
	}

	/// @brief EntitySystem型ごとの記述子の実体。
	/// @details 生成コードが型ごとの.cppで明示的実体化し、その.rdataへ置く。
	///          変数テンプレートなのでCRTP基底からも同じ実体を参照でき、
	///          生成ヘッダをインクルードしなくても記述子の同一性が保たれる。
	template<class TSystem>
	inline constexpr nox::EntitySystemTypeDescriptor k_entity_system_type_descriptor =
		nox::MakeEntitySystemTypeDescriptor<TSystem>();

	/// @brief entityを一括処理するSystemの基底。
	/// @tparam TDerived CRTPの派生型。 void OnUpdate(引数リスト) を実装する。
	/// @tparam _Phase 実行フェーズ。
	template<class TDerived, nox::SystemPhaseType _Phase = nox::SystemPhaseType::Update>
	class EntitySystem : public nox::EntitySystemBase
	{
	public:
		static constexpr nox::SystemPhaseType k_phase = _Phase;

		/// @brief OnUpdateの引数から導出したアクセス宣言。
		/// @details CRTP基底の実体化時点ではTDerivedが未完成なため、使用時まで実体化を遅らせる。
		template<class TSystem = TDerived>
		using SignatureOf = typename nox::EntityMethodTraits<decltype(&TSystem::OnUpdate)>::Signature;

	protected:
		inline EntitySystem()noexcept :
			nox::EntitySystemBase(nox::k_entity_system_type_descriptor<TDerived>)
		{
		}

		inline ~EntitySystem() = default;
	};
}
