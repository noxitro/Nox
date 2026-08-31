// Copyright (C) 2026 NOX ENGINE All rights reserved.

/// @file	entity_system.h
/// @brief	大量のentityを一括処理するSystem。
/// @details 宣言は OnUpdate の引数リストだけ。const参照が読み取り、非const参照が書き込みを意味し、
///          その宣言がそのままUpdaterGraphの依存解析の入力になる。宣言外のComponentDataには
///          そもそも触れられないため、宣言と実装が乖離しない。
///
///          仮想関数は使わない。実行はTDerived::OnUpdateへ直接束縛された静的サンクを経由するため、
///          ForEachのループ内に間接呼び出しが1つも残らない。
#pragma once
#include	"entity_query.h"
#include	"system_phase_type.h"

namespace nox
{
	class World;
	class EntitySystemBase;

	/// @brief EntitySystem型ごとに1つだけ作られる静的記述子。
	/// @details 静的初期化時に連結リストへ繋がれる。ヒープもコンテナも使わない。
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
		/// @brief 実行本体。TDerived::OnUpdateへ静的に束縛されている。
		void (*execute)(nox::EntitySystemBase&, nox::World&);
		std::string_view name;
		nox::SystemPhaseType phase;
		/// @brief 静的初期化順に繋がれる次の記述子。
		const nox::EntitySystemTypeDescriptor* next;
	};

	namespace detail
	{
		/// @brief 記述子を登録リストへ繋ぐ。静的初期化中に呼ばれる。
		void RegisterEntitySystemType(nox::EntitySystemTypeDescriptor& descriptor)noexcept;

		/// @brief 登録済みEntitySystem型の先頭。
		[[nodiscard]] const nox::EntitySystemTypeDescriptor* GetEntitySystemTypeListHead()noexcept;
	}

	/// @brief EntitySystemの非テンプレート基底。Worldはこの型でのみ保持する。
	/// @details 仮想関数を持たない。多態は記述子の関数ポインタで行う。
	class EntitySystemBase
	{
	public:
		EntitySystemBase(const EntitySystemBase&) = delete;
		EntitySystemBase& operator=(const EntitySystemBase&) = delete;

		inline void Execute(nox::World& world) { descriptor_.execute(*this, world); }

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

		/// @brief 型記述子。EntitySystemRegistrarが静的初期化時に参照する。
		[[nodiscard]] static nox::EntitySystemTypeDescriptor& GetTypeDescriptor()noexcept
		{
			using Signature = SignatureOf<>;
			static_assert(nox::detail::ValidateEntityMethod<decltype(&TDerived::OnUpdate)>());
			static nox::EntitySystemTypeDescriptor descriptor{
				.create = []() -> nox::EntitySystemBase* { return new TDerived(); },
				.destroy = [](nox::EntitySystemBase* instance)noexcept { delete static_cast<TDerived*>(instance); },
				.make_read_write_mask = []()noexcept { return Signature::GetReadWriteMask(); },
				.make_write_mask = []()noexcept { return Signature::GetWriteMask(); },
				.execute = &EntitySystem::ExecuteImpl,
				.name = nox::util::GetTypeName<TDerived>(),
				.phase = _Phase,
				.next = nullptr,
			};
			return descriptor;
		}

	protected:
		inline EntitySystem()noexcept :
			nox::EntitySystemBase(GetTypeDescriptor())
		{
		}

		inline ~EntitySystem() = default;

	private:
		static void ExecuteImpl(nox::EntitySystemBase& self, nox::World& world)
		{
			using Signature = SignatureOf<>;
			auto& derived = static_cast<TDerived&>(self);
			nox::detail::EntityInvokerOf<Signature>::ForEachEntity(world, self.GetQuery(), derived, &TDerived::OnUpdate);
		}
	};

	/// @brief 静的初期化時に型記述子を登録する。NOX_DECLARE_ENTITY_SYSTEMが埋め込む。
	template<class TSystem>
	struct EntitySystemRegistrar final
	{
		inline EntitySystemRegistrar()noexcept
		{
			nox::detail::RegisterEntitySystemType(TSystem::GetTypeDescriptor());
		}
	};
}

/// @brief EntitySystemを購読させる。クラス本体に1行書くだけでWorldが自動生成・自動実行する。
/// @details 静的初期化で記述子が連結リストに繋がるため、明示的な登録呼び出しは不要。
#define NOX_DECLARE_ENTITY_SYSTEM(type)														\
	private:																				\
		friend struct ::nox::EntitySystemRegistrar<type>;									\
		static inline const ::nox::EntitySystemRegistrar<type> k_nox_entity_system_registrar_{};	\
	public:
//	end define
