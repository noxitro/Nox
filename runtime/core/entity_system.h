// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

/// @file	entity_system.h
/// @brief	System
#pragma once
#include	"ecs_definitions.h"
#include	"entity.h"

namespace nox
{
	class World;

	/// @brief		ECSのSystem
	///				OnUpdate,OnAdd,OnRemoveを提供する
	/// @details	インスタンス化は不可。
	struct EntitySystemBase : public nox::detail::ISystemBase
	{
		constexpr EntitySystemBase()noexcept = delete;
	};

	namespace concepts
	{
		/// @brief ECSのSystemか
		template<class T>
		concept EntitySystem = std::is_same_v<T, nox::EntitySystemBase> == false && std::is_base_of_v<nox::EntitySystemBase, T>;
	}

	template<class T,class ExtraRequiredComponents = nox::type_identities_t<>, class AfterSystems = nox::type_identities_t<>, class BeforeSystems = nox::type_identities_t<>>
	struct EntitySystem;

	/// @brief 
	/// @tparam T 
	/// @tparam ...ExtraRequiredComponents 追加で依存するComponentData型リスト（これらが揃ったentityのみを対象にする）
	/// @tparam ...AfterSystems 依存するシステムリスト（これらを待つ)
	/// @tparam ...BeforeSystems 依存させるシステムリスト（これらを待たせる)
	template<class T, nox::concepts::Component... ExtraRequiredComponents, nox::concepts::EntitySystem... AfterSystems, nox::concepts::EntitySystem... BeforeSystems>
	struct EntitySystem<T, nox::type_identities_t<ExtraRequiredComponents...>, nox::type_identities_t<AfterSystems...>, nox::type_identities_t<BeforeSystems...>> : public nox::EntitySystemBase
	{
		static constexpr bool HasOnUpdate = requires{ &T::OnUpdate; };
		static constexpr bool HasOnAdd = requires{ &T::OnAdd; };
		static constexpr bool HasOnRemove = requires{ &T::OnRemove; };

	private:
		template<class U>
		static consteval bool CheckFunction()noexcept
		{
			static_assert(std::is_void_v<nox::function_result_t<U>>, "OnUpdate/OnAdd/OnRemove must return void");
			static_assert(std::is_function_v<std::remove_pointer_t<U>>, "OnUpdate/OnAdd/OnRemove must be function pointer");
			return true;
		}

	protected:
		static consteval bool StaticDeclareVerify()noexcept
		{
			if constexpr (HasOnUpdate)
			{
				static_assert(CheckFunction<decltype(&T::OnUpdate)>());
			}
			if constexpr (HasOnAdd)
			{
				static_assert(CheckFunction<decltype(&T::OnAdd)>());
			}
			if constexpr (HasOnRemove)
			{
				static_assert(CheckFunction<decltype(&T::OnRemove)>());
			}

			return true;
		}
	};
}