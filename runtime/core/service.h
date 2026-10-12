// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

/// @file	service.h
/// @brief	service
/// @details Worldが所有する共有機能。Worldの破棄がそのままServiceの破棄になるため、
///          シングルトンのような特別な終了処理・再初期化を持たない。
///          System / EntityLogic は引数に Service* を書くだけで受け取れる。
#pragma once
#include	"updater_node.h"
#include	"node_descriptor.h"

namespace nox
{
	class World;

	namespace detail
	{
		/// @brief	System,EntityLogic,Service間で参照可能な機能
		///			Systemなどと同じ実行ノードを持ち、Init,Update,Terminateの各フェーズを定義可能
		///			Phase関数の引数は、ServiceとExtraResrouceのみ定義可能
		class ServiceBase : public nox::detail::UpdaterNodeOwnerBase
		{
		public:

		protected:
			
			template<class Func, nox::detail::updater_graph::NodeTrigger trigger>
			struct PhaseImpl
			{
				const Func function;
				static constexpr nox::detail::updater_graph::NodeTrigger phase_type = trigger;
			};

			template<auto/*PhaseImpl*/ Phase, class... Options>
			using Register = nox::detail::updater_graph::Register<Phase.function, Phase.phase_type, Options...>;

		public:
			template<class Function> requires (std::is_member_function_pointer_v<Function>)
			struct PhaseInit final : PhaseImpl<Function, nox::detail::updater_graph::NodeTrigger::Init> {};

			template<class Function> requires (std::is_member_function_pointer_v<Function>)
			struct PhaseUpdate final : PhaseImpl<Function, nox::detail::updater_graph::NodeTrigger::Update> {};

			template<class Function> requires (std::is_member_function_pointer_v<Function>)
			struct PhaseTerminate final : PhaseImpl<Function, nox::detail::updater_graph::NodeTrigger::Terminate> {};

			template<class Func>
			PhaseInit(Func) -> PhaseInit<Func>;

			template<class Func>
			PhaseUpdate(Func) -> PhaseUpdate<Func>;

			template<class Func>
			PhaseTerminate(Func) -> PhaseTerminate<Func>;
		};
	}

	template<class T>
	class Service : public nox::detail::ServiceBase
	{
	public:
		////	static constexpr auto GetPhaseList()noexcept {}
		///// @brief (a) の既定。(b) の型は同じ名前の関数を宣言してこれを隠す
		//static const NodeDescriptor& GetNodeDescriptor()noexcept
		//	requires requires { typename T::RegisterList; }
		//{
		//	return nox::node_descriptor_v<T, typename T::RegisterList>;
		//}
	protected:
		/// @brief	自分の型 T を埋めた記述子。GetNodeDescriptor() の本体で node_descriptor_v<Register<kInit>, ...> と書く
		template<class... Registers>
		static constexpr const nox::NodeDescriptor& node_descriptor_v = nox::detail::NodeDescriptorBuilder<T, std::tuple<Registers...>>::value;

		static consteval bool StaticDeclareVerify()noexcept
		{
			static_assert(std::is_polymorphic_v<T> == false, "Service must not be polymorphic");
			static_assert(requires { T::GetNodeDescriptor(); }, "Service must have a valid NodeDescriptor");
			return true;
		}
	};
}