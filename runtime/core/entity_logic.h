//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	entity_logic.h
///	@brief	entity_logic
#pragma once
#include	"entity.h"
#include	"node_descriptor.h"

namespace nox
{
	namespace util
	{
	}

	namespace detail
	{
		class EntityLogicBase : public nox::detail::UpdaterNodeOwnerBase
		{
		public:
			/// @brief メソッドの呼び出しフェーズ。EntityLogicの更新メソッドはこのいずれかで呼ばれる
			enum class Trigger : nox::uint8
			{
				Update,
				Add,
				Remove,
			};

		private:
			static inline consteval nox::detail::updater_graph::NodeTrigger ToNodeTrigger(Trigger trigger)noexcept
			{
				switch (trigger)
				{
				case Trigger::Update: return nox::detail::updater_graph::NodeTrigger::Update;
				case Trigger::Add: return nox::detail::updater_graph::NodeTrigger::Add;
				case Trigger::Remove: return nox::detail::updater_graph::NodeTrigger::Remove;
				default: return nox::detail::updater_graph::NodeTrigger::Update;
				}
			}

		protected:
			template<auto Method, nox::detail::EntityLogicBase::Trigger trigger, class... Options>
			using Register = nox::detail::updater_graph::Register<Method, ToNodeTrigger(trigger), Options...>;

			constexpr EntityLogicBase()noexcept = default;
		};
	}

	/// @brief メンバ変数を持てる振る舞いクラス
	/// @tparam T 
	/// @tparam ...Options RunAfter / RunBefore / RequireComponents / ExcludeComponents などのオプション
	template<class T, class... Options>
	class EntityLogic : public nox::detail::EntityLogicBase
	{
	protected:
		template<class... Registers>
		static constexpr const nox::NodeDescriptor& node_descriptor_v = nox::detail::NodeDescriptorBuilder<T, std::tuple<Registers...>>::value;

		static consteval bool StaticDeclareVerify()noexcept
		{
			static_assert(std::is_polymorphic_v<T> == false, "EntityLogicは仮想関数を持てません");
			static_assert(requires { T::GetNodeDescriptor(); }, "EntityLogic must have a valid NodeDescriptor");
			return true;
		}
	};
}