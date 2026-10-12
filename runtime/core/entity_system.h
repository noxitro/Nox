// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

/// @file	entity_system.h
/// @brief	System
#pragma once
#include	"ecs_definitions.h"
#include	"node_descriptor.h"
#include	"entity.h"

namespace nox
{
	namespace detail
	{
		/// @brief		ECSのSystem
		///				OnUpdate,OnAdd,OnRemoveを提供する
		/// @details	インスタンス化は不可。
		struct EntitySystemBase : nox::detail::UpdaterNodeOwnerBase
		{
			constexpr EntitySystemBase()noexcept = delete;
		};

		/// @brief	trigger に対応するメソッド (OnUpdate / OnAdd / OnRemove) のポインタ。定義していなければ nullptr
		/// @note	public でないメソッドは見えない (定義していない扱いになる)
		template<class T, nox::detail::updater_graph::NodeTrigger trigger>
		consteval auto GetEntitySystemMethod()noexcept
		{
			using enum nox::detail::updater_graph::NodeTrigger;
			if constexpr (trigger == Update && requires { &T::OnUpdate; })
			{
				return &T::OnUpdate;
			}
			else if constexpr (trigger == Add && requires { &T::OnAdd; })
			{
				return &T::OnAdd;
			}
			else if constexpr (trigger == Remove && requires { &T::OnRemove; })
			{
				return &T::OnRemove;
			}
			else
			{
				return nullptr;
			}
		}

		/// @brief	trigger のメソッドを定義していれば Register 1 つ、無ければ空の tuple
		template<class T, nox::detail::updater_graph::NodeTrigger trigger>
		using entity_system_register_t = std::conditional_t<
			std::is_null_pointer_v<decltype(nox::detail::GetEntitySystemMethod<T, trigger>())>,
			std::tuple<>,
			std::tuple<nox::detail::updater_graph::Register<nox::detail::GetEntitySystemMethod<T, trigger>(), trigger>>
		>;

		template<class UpdateList, class AddList, class RemoveList>
		struct entity_system_register_list;

		template<class... Update, class... Add, class... Remove>
		struct entity_system_register_list<std::tuple<Update...>, std::tuple<Add...>, std::tuple<Remove...>> final
		{
			using type = std::tuple<Update..., Add..., Remove...>;
		};

		/// @brief	T が定義した OnUpdate / OnAdd / OnRemove を、この順に並べた Register の tuple
		template<class T>
		using entity_system_register_list_t = typename nox::detail::entity_system_register_list<
			nox::detail::entity_system_register_t<T, nox::detail::updater_graph::NodeTrigger::Update>,
			nox::detail::entity_system_register_t<T, nox::detail::updater_graph::NodeTrigger::Add>,
			nox::detail::entity_system_register_t<T, nox::detail::updater_graph::NodeTrigger::Remove>
		>::type;
	}

	namespace concepts
	{
		/// @brief ECSのSystemか
		template<class T>
		concept EntitySystem =
			std::is_same_v<T, nox::detail::EntitySystemBase> == false &&
			std::is_base_of_v<nox::detail::EntitySystemBase, T>;
	}

	/// @brief		ECSのSystemの基底
	/// @details	static の OnUpdate / OnAdd / OnRemove を public に定義すると、定義したものだけが自動で登録される (RegisterList は書かない)
	/// @tparam T
	/// @tparam ...Options RequireComponents / ExcludeComponents / RunAfter / RunBefore (順番は自由)
	template<class T, class... Options>
	struct EntitySystem : public nox::detail::EntitySystemBase
	{
	private:
		template<nox::detail::updater_graph::NodeTrigger trigger>
		static consteval bool CheckMethod()noexcept
		{
			using MethodType = decltype(nox::detail::GetEntitySystemMethod<T, trigger>());
			if constexpr (std::is_null_pointer_v<MethodType> == false)
			{
				static_assert(std::is_function_v<std::remove_pointer_t<MethodType>>, "OnUpdate/OnAdd/OnRemove must be static");
				static_assert(std::is_void_v<nox::function_result_t<MethodType>>, "OnUpdate/OnAdd/OnRemove must return void");
			}
			return true;
		}

	protected:
		static consteval bool StaticDeclareVerify()noexcept
		{
			static_assert(CheckMethod<nox::detail::updater_graph::NodeTrigger::Update>());
			static_assert(CheckMethod<nox::detail::updater_graph::NodeTrigger::Add>());
			static_assert(CheckMethod<nox::detail::updater_graph::NodeTrigger::Remove>());
			static_assert(std::tuple_size_v<nox::detail::entity_system_register_list_t<T>> > 0, "EntitySystem must define public static OnUpdate, OnAdd or OnRemove");
			return true;
		}

	public:
		static constexpr const nox::NodeDescriptor& GetNodeDescriptor()noexcept
		{
			return nox::detail::NodeDescriptorBuilder<T, nox::detail::entity_system_register_list_t<T>>::value;
		}
	};
}