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

	struct IComponentData;

	namespace detail
	{
		struct IECSBase
		{
		};

		struct ISystemBase : public nox::detail::IECSBase
		{
			template<class... Types>
			struct EntityAccess final
			{
				inline constexpr explicit EntityAccess(nox::World& world)noexcept : world_(world) {}

				template<class T> requires ((std::is_same_v<T, Types> || ...))
					[[nodiscard]] inline T& Get()const noexcept
				{
					T* dummy = nullptr;
					return *dummy;
				}
			private:
				nox::World& world_;
			};
		};
	}

	/// @brief		ECSのSystem
	///				OnUpdate,OnAdd,OnRemoveを提供する
	/// @details	インスタンス化は不可。
	struct EntitySystemBase : public nox::detail::ISystemBase
	{
		constexpr EntitySystemBase()noexcept = delete;
	};

	template<class... Types>
	struct RequireComponents {};

	template<class... Types>
	struct RunAfter{};

	template<class... Types>
	struct RunBefore{};

	namespace detail
	{
		template<class T>
		struct is_require_components : std::false_type {};

		template<class... Types>
		struct is_require_components<nox::RequireComponents<Types...>> : std::true_type {};

		template<class T>
		struct is_run_after : std::false_type {};

		template<class... Types>
		struct is_run_after<nox::RunAfter<Types...>> : std::true_type {};

		template<class T>
		struct is_run_before : std::false_type {};

		template<class... Types>
		struct is_run_before<nox::RunBefore<Types...>> : std::true_type {};

		template<class T>
		constexpr static bool is_require_components_v = nox::detail::is_require_components<T>::value;

		template<class T>
		constexpr static bool is_run_after_v = nox::detail::is_run_after<T>::value;

		template<class T>
		constexpr static bool is_run_before_v = nox::detail::is_run_before<T>::value;
	}

	namespace concepts
	{
		/// @brief ECSのSystemか
		template<class T>
		concept EntitySystem = std::is_same_v<T, nox::EntitySystemBase> == false && std::is_base_of_v<nox::EntitySystemBase, T>;
	}

	/// @brief 
	/// @tparam T 
	/// @tparam ...ExtraRequiredComponents 追加で依存するComponentData型リスト（これらが揃ったentityのみを対象にする）
	/// @tparam ...AfterSystems 依存するシステムリスト（これらを待つ)
	/// @tparam ...BeforeSystems 依存させるシステムリスト（これらを待たせる)
	template<class T, class... Options>
	struct EntitySystem : public nox::EntitySystemBase
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
	};
}