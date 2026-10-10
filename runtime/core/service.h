// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

/// @file	service.h
/// @brief	service
/// @details Worldが所有する共有機能。Worldの破棄がそのままServiceの破棄になるため、
///          シングルトンのような特別な終了処理・再初期化を持たない。
///          System / EntityLogic は引数に Service* を書くだけで受け取れる。
#pragma once
#include	"entity_options.h"

namespace nox
{
	template<class T>
	struct NodeDescriptor
	{

	};

	template<class T>
	const nox::NodeDescriptor<T>& GetNodeDescriptor()noexcept;

	class World;

	namespace detail
	{
		/// @brief	System,EntityLogic,Service間で参照可能な機能
		///			Systemなどと同じ実行ノードを持ち、Init,Update,Terminateの各フェーズを定義可能
		///			Phase関数の引数は、ServiceとExtraResrouceのみ定義可能
		class ServiceBase : public nox::detail::UpdaterNodeOwnerBase
		{
		public:
			enum class PhaseType : nox::uint8
			{
				Init,
				Update,
				Terminate,
			};

		protected:
			struct Phase
			{
			};

			template<PhaseType phase_type, auto Func>
			struct PhaseImpl : public Phase
			{

			};

			template<auto Func>
			using PhaseInit = PhaseImpl<PhaseType::Init, Func>;
			template<auto Func>
			using PhaseUpdate = PhaseImpl<PhaseType::Update, Func>;
			template<auto Func>
			using PhaseTerminate = PhaseImpl<PhaseType::Terminate, Func>;

			template<class... Phases>
			struct PhaseRegister;
		};
	}

	template<class T>
	class Service : public nox::detail::ServiceBase
	{
	public:
		//	static constexpr auto GetPhaseList()noexcept {}

	protected:
		static consteval bool StaticDeclareVerify()noexcept
		{
			static_assert(std::is_polymorphic_v<T> == false, "Service must not be polymorphic");
			// GetPhaseList()を実装しているかチェック
			static_assert(requires { T::GetPhaseList(); }, "Service must implement GetPhaseList()");
			return true;
		}
	};
}