// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

/// @file	service_method.h
/// @brief	Serviceの属性付きメソッドを UpdaterGraph のノードにするための記述子。
/// @details Serviceは Update を持たない。毎フレームの処理は、属性
///          NOX_ATTR(nox::attr::ServiceMethod(フェーズ[, 実行スレッド])) を付けたメソッドとして書く。
///          メソッドは private のままでよい(friendも要らない。仕組みは nox::gen::PrivateServiceMethodInvoker)。
///
///          [ノードとしての規則]
///          - 1フェーズに1回だけ呼ばれる。entityを列挙しない(entityが0個でも呼ばれる)。
///          - 引数に取れるのは Service(参照 / ポインタ、constなら読み取り)と nox::EntityCommands& だけ。
///            ComponentData・nox::EntityId は static_assert で弾く(nox::detail::ValidateOncePerFrameSignature)。
///          - 自分自身の Service への書き込みを暗黙に宣言に含める。引数に書かなくても、
///            そのServiceを読み書きする他のノードとは直列化される。
///          - 同じServiceインスタンスのメソッド同士は、宣言が重ならなくても必ず直列化する
///            (EntityLogicの同じ型のメソッド同士と同じ扱い。順序はメソッド名順)。
///          - 型に public な `using RunAfter = nox::TypeList<...>;` / `using RunBefore = ...;` を書ける。
///            その型の同じフェーズの全メソッドに掛かる。
///          - 実行スレッドはメソッド単位で属性に書く(nox::attr::ThreadAffinity)。型の kMainThreadOnly は使わない。
///
///          [生成器との境界]
///          生成器は nox::Service の派生型で属性の付いたメソッドを数え上げ、nox::ServiceMethodTable の
///          明示的特殊化と、型ごとの記述子(nox::MakeServiceMethodTypeDescriptor)を書き出して
///          nox::GetServiceMethodTypes() の表に載せる。引数の妥当性などの判断は全てこのヘッダの static_assert が行う。
///
///          Worldは表に載った型のうち、Worldに登録されているServiceだけをノードにする(nox::World::Init)。
#pragma once
#include	"entity_query.h"
#include	"system_phase_type.h"
#include	"service_attribute.h"

namespace nox
{
	class World;

	/// @brief Serviceの属性付きメソッド1つ分の記述子。
	struct ServiceMethodDescriptor final
	{
		/// @brief TServiceのメソッドへ静的に束縛された呼び出し。仮想関数を介さない。
		/// @details serviceは記述子の型のインスタンス。基底の参照から static_cast で派生型へ戻す。
		void (*invoke)(nox::Service& service, nox::World& world);
		/// @brief 読み書きするServiceの一覧。末尾に自分自身への書き込みを含む。確保は走らない。
		std::span<const nox::ServiceAccess>(*get_service_accesses)()noexcept;
		std::string_view name;
		nox::SystemPhaseType phase;
		/// @brief ワーカーへ配らず、フェーズを回しているスレッド上で実行するか(nox::attr::ThreadAffinity::MainThread)。
		bool main_thread_only;
		/// @brief 遅延構造変更を出しうるか(= nox::EntityCommands& を宣言しているか)。
		bool emits_structural_change;
	};

	/// @brief 属性付きメソッドを持つService型ごとに1つだけ作られる静的記述子。
	/// @details 全メンバが定数式で埋まるため定数初期化される。
	///          nox::ServiceTypeDescriptor(寿命と Depends)とは別に持つ。こちらはメソッド表を読むので、
	///          メソッド表の特殊化が見える翻訳単位(生成コード)でしか作れないため。
	struct ServiceMethodTypeDescriptor final
	{
		/// @brief Serviceの型情報。Worldに登録されたインスタンスとの照合に使う。
		const nox::reflection::Type* type;
		/// @brief 属性付きメソッドの一覧。
		std::span<const nox::ServiceMethodDescriptor>(*get_methods)()noexcept;
		/// @brief 完全修飾型名。UpdaterGraphの全順序のキーと明示辺の名前解決に使う。
		std::string_view name;
		/// @brief この型のメソッドより先に実行する型の完全修飾名(TService::RunAfter)。型の全メソッドに掛かる。
		std::span<const std::string_view> run_after;
		/// @brief この型のメソッドより後に実行する型の完全修飾名(TService::RunBefore)。
		std::span<const std::string_view> run_before;
	};

	/// @brief Service型の属性付きメソッド表。
	/// @details 一次テンプレートは宣言のみ。リフレクション生成コードが、属性付きメソッドを1つ以上持つ型にだけ
	///          明示的特殊化(static constexpr k_methods[] と GetMethods())を定義する。
	///          特殊化の無い型で使うとコンパイルエラーになる(空の表を黙って返さない。
	///          生成コードと別の翻訳単位で別の中身の実体ができるのを防ぐため)。
	///          生成器が名前を書けない型は、この特殊化を手書きすれば記述子を作れる
	///          (ただし nox::GetServiceMethodTypes() の表には載らない。nox::EntityLogicMethodTable と同じ)。
	template<class TService>
	struct ServiceMethodTable;

	namespace detail
	{
		/// @brief Serviceのメソッドとして妥当な形か。
		/// @details 形(戻り値void・volatile / 参照修飾なし)と引数の分類は EntityLogic と同じ検査を通し、
		///          そのうえで 1フェーズに1回のノードの引数規則を課す。
		template<class MethodPointerType>
		[[nodiscard]] consteval bool ValidateServiceMethod()noexcept
		{
			constexpr bool k_valid_form = nox::detail::ValidateEntityMethod<MethodPointerType>();
			if constexpr (k_valid_form)
			{
				using Traits = nox::EntityMethodTraits<MethodPointerType>;
				static_assert(std::derived_from<typename Traits::OwnerType, nox::Service>,
					"nox::attr::ServiceMethod は nox::Service の派生型のメソッドにのみ付けられます");
				return std::derived_from<typename Traits::OwnerType, nox::Service> &&
					nox::detail::ValidateOncePerFrameSignature<typename Traits::Signature>();
			}
			else
			{
				return false;
			}
		}

		/// @brief Serviceのメソッドが読み書きするServiceの表。引数の宣言の後ろに、自分自身への書き込みを1つ足す。
		/// @details 名前は nox::reflection::Typeof のアドレス(定数式)だけなので、表は定数初期化される。
		///          ヒープも動的初期化も使わない。引数に自分自身を並べていても二重に載るだけで害は無い
		///          (衝突判定も並列実行チェッカーも、同じ型の書き込みが1つでもあれば書き込みとして扱う)。
		template<class TService, class Signature>
		struct ServiceMethodAccessTable final
		{
			static constexpr size_t kCount = static_cast<size_t>(Signature::k_service_parameter_count) + 1u;

			[[nodiscard]] static constexpr std::array<nox::ServiceAccess, kCount> MakeAccessArray()noexcept
			{
				std::array<nox::ServiceAccess, kCount> accesses{};
				const std::array<nox::ServiceAccess, Signature::k_service_parameter_count> declared =
					Signature::MakeServiceAccessArray();
				std::copy(declared.begin(), declared.end(), accesses.begin());
				accesses[kCount - 1u] = nox::ServiceAccess{
					.type = &nox::reflection::Typeof<TService>(),
					.write = true,
				};
				return accesses;
			}

			[[nodiscard]] static std::span<const nox::ServiceAccess> Get()noexcept
			{
				static constexpr std::array<nox::ServiceAccess, kCount> k_accesses = MakeAccessArray();
				return std::span<const nox::ServiceAccess>(k_accesses.data(), k_accesses.size());
			}
		};
	}

	/// @brief Serviceのメソッド1つ分の記述子を作る。手書きの特殊化(エスケープハッチ)用。
	/// @details メソッドのアドレスを通常の文脈で取るため、publicなメソッドにしか使えない。
	///          生成コードは nox::detail::MakeServiceMethodDescriptorViaTag を使う。
	template<auto MethodPointer, nox::SystemPhaseType _Phase, nox::attr::ThreadAffinity _Affinity = nox::attr::ThreadAffinity::Any>
	[[nodiscard]] constexpr nox::ServiceMethodDescriptor MakeServiceMethodDescriptor(const std::string_view name)noexcept
	{
		static_assert(nox::detail::ValidateServiceMethod<decltype(MethodPointer)>());

		using Traits = nox::EntityMethodTraits<decltype(MethodPointer)>;
		using OwnerType = typename Traits::OwnerType;
		using Signature = typename Traits::Signature;
		using Invoker = nox::detail::EntityInvokerOf<Signature>;

		return nox::ServiceMethodDescriptor{
			.invoke = [](nox::Service& service, nox::World& world)
				{
					Invoker::InvokeOnce(world, static_cast<OwnerType&>(service), MethodPointer);
				},
			.get_service_accesses = []()noexcept { return nox::detail::ServiceMethodAccessTable<OwnerType, Signature>::Get(); },
			.name = name,
			.phase = _Phase,
			.main_thread_only = (_Affinity == nox::attr::ThreadAffinity::MainThread),
			.emits_structural_change = (Signature::k_commands_parameter_count != 0u),
		};
	}

	/// @brief Service型の、メソッドをノードにするための記述子を作る。
	/// @details nox::ServiceMethodTable<TService> と、あれば TService::RunAfter / RunBefore だけを読む。
	///          メソッド表の特殊化が見える翻訳単位で呼ぶこと(生成コードは型ごとの .g.cpp で呼ぶ)。
	template<class TService>
		requires(std::derived_from<TService, nox::Service>)
	[[nodiscard]] constexpr nox::ServiceMethodTypeDescriptor MakeServiceMethodTypeDescriptor()noexcept
	{
		using MethodTable = nox::ServiceMethodTable<TService>;

		static_assert(MethodTable::GetMethods().empty() == false,
			"nox::ServiceMethodTable にメソッドが1つもありません(属性付きメソッドの無いServiceは記述子を作らなくてよい)");
		//	実行スレッドはメソッド単位で属性に書く。型に書いた kMainThreadOnly は読まないので、黙って無視せずに弾く。
		static_assert((requires { TService::kMainThreadOnly; }) == false,
			"Serviceの実行スレッドはメソッドごとに nox::attr::ServiceMethod の第2引数(nox::attr::ThreadAffinity)で指定してください"
			"(kMainThreadOnly は EntitySystem / EntityLogic 用です)");

		return nox::ServiceMethodTypeDescriptor{
			.type = &nox::reflection::Typeof<TService>(),
			.get_methods = []()noexcept { return MethodTable::GetMethods(); },
			.name = nox::util::GetTypeName<TService>(),
			.run_after = nox::detail::GetRunAfterTypeNames<TService>(),
			.run_before = nox::detail::GetRunBeforeTypeNames<TService>(),
		};
	}
}

namespace nox::gen
{
	/// @brief private なServiceのメソッドを、対象クラスにfriendを足さずにノードへ載せるための実行サンク。
	/// @details 仕組みは nox::gen::PrivateEntityLogicMethodInvoker と同じ([temp.explicit] により、
	///          明示的実体化の宣言に現れる名前にはアクセス検査が適用されない)。
	///          この実体化が Tag に宣言された friend 関数(=実行サンク)を定義する。
	///          記述子側は定数式で friend を呼ばず、invokeラムダの中から実行時に呼ぶ(MSVCの制約。同上)。
	/// @tparam Tag 生成コードが宣言するメソッド1つ分のタグ型。
	/// @tparam MethodPointer 対象メソッドへのメンバ関数ポインタ。
	template<class Tag, auto MethodPointer>
	struct PrivateServiceMethodInvoker final
	{
		using OwnerType = typename nox::EntityMethodTraits<decltype(MethodPointer)>::OwnerType;
		using Signature = typename nox::EntityMethodTraits<decltype(MethodPointer)>::Signature;

		//	依存型を引数に取る friend のため、汎用リフレクションの対象からは外す。
		NOX_ATTR(nox::reflection::attr::IgnoreReflection())
		friend void InvokeServiceMethod(Tag, nox::Service& service, nox::World& world)
		{
			nox::detail::EntityInvokerOf<Signature>::InvokeOnce(world, static_cast<OwnerType&>(service), MethodPointer);
		}
	};
}

namespace nox::detail
{
	/// @brief Serviceのメソッド1つ分の記述子を、生成コードのタグ経由で作る。
	/// @details メソッドが public でも private でも同じ経路に載る。記述子はメソッド名を一切綴らず、
	///          Tag が持つ型情報(所有型・メンバ関数ポインタ型)だけを読む。
	/// @tparam Tag 生成コードが宣言したタグ型。OwnerType / MethodPointerType / Signature を持つ。
	template<class Tag, nox::SystemPhaseType _Phase, nox::attr::ThreadAffinity _Affinity>
	[[nodiscard]] constexpr nox::ServiceMethodDescriptor MakeServiceMethodDescriptorViaTag(const std::string_view name)noexcept
	{
		using OwnerType = typename Tag::OwnerType;
		using Signature = typename Tag::Signature;

		//	引数リストの妥当性検査は手書き経路と同一。
		static_assert(nox::detail::ValidateServiceMethod<typename Tag::MethodPointerType>());

		return nox::ServiceMethodDescriptor{
			.invoke = [](nox::Service& service, nox::World& world)
				{
					//	定数評価されるのはラムダ→関数ポインタ変換だけ。friendの呼び出しは実行時。
					InvokeServiceMethod(Tag{}, service, world);
				},
			.get_service_accesses = []()noexcept { return nox::detail::ServiceMethodAccessTable<OwnerType, Signature>::Get(); },
			.name = name,
			.phase = _Phase,
			.main_thread_only = (_Affinity == nox::attr::ThreadAffinity::MainThread),
			.emits_structural_change = (Signature::k_commands_parameter_count != 0u),
		};
	}
}
