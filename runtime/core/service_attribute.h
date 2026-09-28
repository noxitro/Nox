//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	service_attribute.h
///	@brief	Serviceのメソッドとグローバル関数を UpdaterGraph のノードにする属性。
///	@details	どちらも「1フェーズに1回だけ呼ばれ、entityを列挙しない」ノードになる。
///				引数の規則・記述子は service_method.h / updater_task.h を参照。
#pragma once
#include	"attribute.h"
#include	"system_phase_type.h"

namespace nox::attr
{
	/// @brief		ノードを実行するスレッドの指定。
	/// @details	MainThread は、ワーカーへ配らずフェーズを回しているスレッド上で実行する
	///				(EntitySystem / EntityLogic の kMainThreadOnly と同じ扱い。nox::IsMainThreadOnlyUpdaterType を参照)。
	///				依存解析(レイヤー)には影響しない。
	enum class ThreadAffinity : nox::uint8
	{
		/// @brief どのスレッドで実行してもよい(既定)。
		Any,
		/// @brief フェーズを回しているスレッド上でだけ実行する(OSのメッセージを読む処理など)。
		MainThread,
	};

	/// @brief		Serviceのメソッドを UpdaterGraph のノードとして購読させる。
	/// @details	nox::Service の派生型のメソッド(privateでよい)にだけ付けられる。
	///				この属性を付けたメソッドだけが、リフレクション生成コードの作る nox::ServiceMethodTable に載る。
	///				フェーズと実行スレッドはここで指定した値が生成コード側でコンパイル時に読み出される
	///				(生成器は引数を解釈しない)。実行スレッドはメソッド単位で決める
	///				(入力のポーリングだけをメインスレッドにし、他のメソッドはワーカーへ配る、が書ける)。
	///				@code
	///				class InputService final : public nox::Service
	///				{
	///				    NOX_DECLARE_OBJECT(game::InputService, nox::Service);
	///				private:
	///				    NOX_ATTR(nox::attr::ServiceMethod(nox::SystemPhaseType::Update, nox::attr::ThreadAffinity::MainThread))
	///				    void Poll();
	///				};
	///				@endcode
	class
		NOX_ATTR_TYPE(::nox::attr::AttributeUsage(nox::attr::AttributeTargets::Function))
		ServiceMethod : public nox::attr::Attribute
	{
		NOX_DECLARE_OBJECT(ServiceMethod, nox::attr::Attribute);
	public:
		inline constexpr explicit ServiceMethod(
			const nox::SystemPhaseType phase,
			const nox::attr::ThreadAffinity affinity = nox::attr::ThreadAffinity::Any)noexcept :
			phase_(phase),
			affinity_(affinity)
		{
		}

		[[nodiscard]] inline constexpr nox::SystemPhaseType GetPhase()const noexcept { return phase_; }
		[[nodiscard]] inline constexpr nox::attr::ThreadAffinity GetThreadAffinity()const noexcept { return affinity_; }

	private:
		const nox::SystemPhaseType phase_;
		const nox::attr::ThreadAffinity affinity_;
	};

	/// @brief		名前空間スコープの関数を UpdaterGraph のノード(Task)として購読させる。
	/// @details	状態を持たない「1フェーズに1回」の処理を書く場所。戻り値は void、
	///				引数は Service(参照 / ポインタ)と nox::EntityCommands& だけ(nox::MakeUpdaterTaskDescriptor を参照)。
	///				無名名前空間の関数・関数テンプレート・メンバ関数には付けられない(生成器がエラーにする)。
	///
	///				Task は RunAfter / RunBefore を持たない(型ではないので書く場所が無い)。
	///				他のノードの RunAfter / RunBefore に並べることもできない。
	///				他のノードとの順序を明示したい処理は、Serviceのメソッド(nox::attr::ServiceMethod)にすること。
	///				@code
	///				NOX_ATTR(nox::attr::UpdaterTask(nox::SystemPhaseType::Update))
	///				void TickScore(game::ScoreService& score, const game::TimeService& time);
	///				@endcode
	class
		NOX_ATTR_TYPE(::nox::attr::AttributeUsage(nox::attr::AttributeTargets::Function))
		UpdaterTask : public nox::attr::Attribute
	{
		NOX_DECLARE_OBJECT(UpdaterTask, nox::attr::Attribute);
	public:
		inline constexpr explicit UpdaterTask(
			const nox::SystemPhaseType phase,
			const nox::attr::ThreadAffinity affinity = nox::attr::ThreadAffinity::Any)noexcept :
			phase_(phase),
			affinity_(affinity)
		{
		}

		[[nodiscard]] inline constexpr nox::SystemPhaseType GetPhase()const noexcept { return phase_; }
		[[nodiscard]] inline constexpr nox::attr::ThreadAffinity GetThreadAffinity()const noexcept { return affinity_; }

	private:
		const nox::SystemPhaseType phase_;
		const nox::attr::ThreadAffinity affinity_;
	};
}
