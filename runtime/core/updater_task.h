// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

/// @file	updater_task.h
/// @brief	属性付きのグローバル関数(Task)を UpdaterGraph のノードにするための記述子。
/// @details 状態を持たない「1フェーズに1回」の処理は、名前空間スコープの関数に
///          NOX_ATTR(nox::attr::UpdaterTask(フェーズ[, 実行スレッド])) を付けて書く。
///          リフレクション生成コードが nox::GetUpdaterTaskDescriptors() の表に載せ、Worldが自動で実行する。
///
///          [ノードとしての規則]
///          - 1フェーズに1回だけ呼ばれる。entityを列挙しない。
///          - 戻り値は void。引数に取れるのは Service(参照 / ポインタ、constなら読み取り)と nox::EntityCommands& だけ
///            (nox::attr::ServiceMethod と同じ規則。nox::detail::ValidateOncePerFrameSignature)。
///          - インスタンスを持たないので、他のノードと状態を共有しない。衝突は引数の宣言だけで決まる。
///          - 全順序のキーは完全修飾関数名。
///          - RunAfter / RunBefore を持たない。型ではないので書く場所が無く、他のノードの RunAfter / RunBefore に
///            並べることもできない。順序が要る処理は Serviceのメソッド(nox::attr::ServiceMethod)にすること。
///          - 無名名前空間の関数・関数テンプレートは生成器が名前を書けないのでエラーになる。
#pragma once
#include	"entity_query.h"
#include	"system_phase_type.h"
#include	"service_attribute.h"

namespace nox
{
	class World;

	/// @brief Task(属性付きのグローバル関数)1つ分の静的記述子。
	/// @details 全メンバが定数式で埋まるため定数初期化される。
	struct UpdaterTaskDescriptor final
	{
		/// @brief 関数へ静的に束縛された呼び出し。
		void (*invoke)(nox::World& world);
		/// @brief 読み書きするServiceの一覧。確保は走らない。
		std::span<const nox::ServiceAccess>(*get_service_accesses)()noexcept;
		/// @brief 完全修飾関数名。UpdaterGraphの全順序のキー。
		std::string_view name;
		nox::SystemPhaseType phase;
		/// @brief ワーカーへ配らず、フェーズを回しているスレッド上で実行するか(nox::attr::ThreadAffinity::MainThread)。
		bool main_thread_only;
		/// @brief 遅延構造変更を出しうるか(= nox::EntityCommands& を宣言しているか)。
		bool emits_structural_change;
	};

	namespace detail
	{
		template<class FunctionPointerType>
		[[nodiscard]] consteval bool IsUpdaterTaskFunctionPointer()noexcept
		{
			if constexpr (std::is_pointer_v<FunctionPointerType>)
			{
				return std::is_function_v<std::remove_pointer_t<FunctionPointerType>>;
			}
			else
			{
				return false;
			}
		}

		/// @brief Taskとして妥当な関数か。
		template<class FunctionPointerType>
		[[nodiscard]] consteval bool ValidateUpdaterTaskFunction()noexcept
		{
			static_assert(nox::detail::IsUpdaterTaskFunctionPointer<FunctionPointerType>(),
				"Taskには名前空間スコープの関数へのポインタを指定してください");

			if constexpr (nox::detail::IsUpdaterTaskFunctionPointer<FunctionPointerType>())
			{
				static_assert(std::is_void_v<nox::FunctionResultType<FunctionPointerType>>,
					"Taskの戻り値は void にしてください");
				using Signature = typename nox::detail::EntitySignatureFromTuple<
					nox::FunctionArgsTupleType<FunctionPointerType>>::Type;
				return std::is_void_v<nox::FunctionResultType<FunctionPointerType>> &&
					nox::detail::ValidateOncePerFrameSignature<Signature>();
			}
			else
			{
				return false;
			}
		}
	}

	/// @brief Task1つ分の記述子を作る。
	/// @details 生成コードは関数を正確な関数ポインタ型へキャストして渡す(オーバーロードで曖昧にならないように)。
	///          手書きで作ることもできる(生成器を通さない関数をテストで直接グラフへ渡すなど)。
	/// @tparam FunctionPointer 名前空間スコープの関数へのポインタ。
	/// @param name 完全修飾関数名。UpdaterGraphの全順序のキーになる。
	template<auto FunctionPointer, nox::SystemPhaseType _Phase, nox::attr::ThreadAffinity _Affinity = nox::attr::ThreadAffinity::Any>
	[[nodiscard]] constexpr nox::UpdaterTaskDescriptor MakeUpdaterTaskDescriptor(const std::string_view name)noexcept
	{
		using FunctionPointerType = decltype(FunctionPointer);
		static_assert(nox::detail::ValidateUpdaterTaskFunction<FunctionPointerType>());

		using Signature = typename nox::detail::EntitySignatureFromTuple<
			nox::FunctionArgsTupleType<FunctionPointerType>>::Type;
		using Invoker = nox::detail::EntityInvokerOf<Signature>;

		return nox::UpdaterTaskDescriptor{
			.invoke = [](nox::World& world)
				{
					Invoker::InvokeFunctionOnce(world, FunctionPointer);
				},
			.get_service_accesses = []()noexcept { return Signature::GetServiceAccesses(); },
			.name = name,
			.phase = _Phase,
			.main_thread_only = (_Affinity == nox::attr::ThreadAffinity::MainThread),
			.emits_structural_change = (Signature::k_commands_parameter_count != 0u),
		};
	}
}
