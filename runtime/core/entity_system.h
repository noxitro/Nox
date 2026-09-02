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
		std::string_view name;
		nox::SystemPhaseType phase;
	};

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
	}

	/// @brief EntitySystem型の記述子を作る。
	/// @details TSystem::k_phase と TSystem::SignatureOf<> だけを読む。CRTP基底には何も持たせない。
	template<class TSystem>
	[[nodiscard]] constexpr nox::EntitySystemTypeDescriptor MakeEntitySystemTypeDescriptor()noexcept
	{
		using Signature = typename TSystem::template SignatureOf<>;
		static_assert(nox::detail::ValidateEntityMethod<decltype(&TSystem::OnUpdate)>());

		return nox::EntitySystemTypeDescriptor{
			.create = []() -> nox::EntitySystemBase* { return new TSystem(); },
			.destroy = [](nox::EntitySystemBase* instance)noexcept { delete static_cast<TSystem*>(instance); },
			.make_read_write_mask = []()noexcept { return Signature::GetReadWriteMask(); },
			.make_write_mask = []()noexcept { return Signature::GetWriteMask(); },
			.get_service_accesses = []()noexcept { return Signature::GetServiceAccesses(); },
			.execute = &nox::detail::ExecuteEntitySystem<TSystem>,
			.name = nox::util::GetTypeName<TSystem>(),
			.phase = TSystem::k_phase,
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
