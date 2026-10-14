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
			constexpr EntitySystemBase(const EntitySystemBase&)noexcept = delete;
			constexpr EntitySystemBase(EntitySystemBase&&)noexcept = delete;
		};
	}

	namespace concepts
	{
		/// @brief ECSのSystemか
		template<class T>
		concept EntitySystem = 
			std::is_same_v<T, nox::detail::EntitySystemBase> == false && 
			std::is_base_of_v<nox::detail::EntitySystemBase, T>;
	}

	/// @brief 
	/// @tparam T 
	/// @tparam ...ExtraRequiredComponents 追加で依存するComponentData型リスト（これらが揃ったentityのみを対象にする）
	/// @tparam ...AfterSystems 依存するシステムリスト（これらを待つ)
	/// @tparam ...BeforeSystems 依存させるシステムリスト（これらを待たせる)
	template<class T, class... Options>
	struct EntitySystem : public nox::detail::EntitySystemBase
	{
		template<class U = T>
		static constexpr bool HasOnUpdate = requires{ &U::OnUpdate; };
		template<class U = T>
		static constexpr bool HasOnAdd = requires{ &U::OnAdd; };
		template<class U = T>
		static constexpr bool HasOnRemove = requires{ &U::OnRemove; };

	private:
		template<class U>
		static consteval bool CheckFunction()noexcept
		{
			static_assert(std::is_void_v<nox::function_result_t<U>>, "OnUpdate/OnAdd/OnRemove must return void");
			static_assert(std::is_function_v<std::remove_pointer_t<U>>, "OnUpdate/OnAdd/OnRemove must be function pointer");
			return true;
		}

	private:
		// 既定: 定義していない
		template<class U, nox::detail::updater_graph::NodeTrigger trigger>
		struct EntitySystemRegister final{ using type = std::tuple<>; };

		// 定義しているときだけ選ばれる。中身は選ばれたときにしか作られないので、&T::OnUpdate を書ける
		template<class U> requires requires { &U::OnUpdate; }
		struct EntitySystemRegister<U, nox::detail::updater_graph::NodeTrigger::Update>final
		{ 
			using type = std::tuple<nox::detail::updater_graph::Register<&U::OnUpdate, nox::detail::updater_graph::NodeTrigger::Update>>; 
		};

		template<class U> requires requires { &U::OnAdd; }
		struct EntitySystemRegister<U, nox::detail::updater_graph::NodeTrigger::Add> final
		{
			using type = std::tuple<nox::detail::updater_graph::Register<&U::OnAdd, nox::detail::updater_graph::NodeTrigger::Add>>;
		};

		template<class U> requires requires { &U::OnRemove; }
		struct EntitySystemRegister<U, nox::detail::updater_graph::NodeTrigger::Remove> final
		{
			using type = std::tuple<nox::detail::updater_graph::Register<&U::OnRemove, nox::detail::updater_graph::NodeTrigger::Remove>>;
		};

		
	protected:
		static consteval bool StaticDeclareVerify()noexcept
		{
			if constexpr (HasOnUpdate<T>)
			{
				static_assert(CheckFunction<decltype(&T::OnUpdate)>());
			}
			if constexpr (HasOnAdd<T>)
			{
				static_assert(CheckFunction<decltype(&T::OnAdd)>());
			}
			if constexpr (HasOnRemove<T>)
			{
				static_assert(CheckFunction<decltype(&T::OnRemove)>());
			}

			return true;
		}
	public:
		static constexpr const nox::NodeDescriptor& GetNodeDescriptor()noexcept
		{
			using RegisterList = nox::tuple_cat_t<
				typename EntitySystemRegister<T, nox::detail::updater_graph::NodeTrigger::Update>::type,
				typename EntitySystemRegister<T, nox::detail::updater_graph::NodeTrigger::Add>::type,
				typename EntitySystemRegister<T, nox::detail::updater_graph::NodeTrigger::Remove>::type>;

			return nox::detail::NodeDescriptorBuilder<T, RegisterList>::value;
		}
	};
}