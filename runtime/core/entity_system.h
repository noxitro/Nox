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

		protected:
			template<class T, bool HasUpdate, bool HasAdd, bool HasRemove>
			struct RegisterList;

			template<class T>
			struct RegisterList<T, true, true, true> final
			{
				static constexpr auto value = std::tuple{
					Register<&T::OnUpdate, Trigger::Update>,
					Register<&T::OnAdd, Trigger::Add>,
					Register<&T::OnRemove, Trigger::Remove>
				};
			};

			template<class T>
			struct RegisterList<T, true, true, false> final
			{
				static constexpr auto value = std::tuple{
					Register<&T::OnUpdate, Trigger::Update>,
					Register<&T::OnAdd, Trigger::Add>
				};
			};

			template<class T>
			struct RegisterList<T, true, false, true> final
			{
				static constexpr auto value = std::tuple{
					Register<&T::OnUpdate, Trigger::Update>,
					Register<&T::OnRemove, Trigger::Remove>
				};
			};

			template<class T>
			struct RegisterList<T, true, false, false> final
			{
				static constexpr auto value = std::tuple{
					Register<&T::OnUpdate, Trigger::Update>
				};
			};

			template<class T>
			struct RegisterList<T, false, true, true> final
			{
				static constexpr auto value = std::tuple{
					Register<&T::OnAdd, Trigger::Add>,
					Register<&T::OnRemove, Trigger::Remove>
				};
			};

			template<class T>
			struct RegisterList<T, false, true, false> final
			{
				static constexpr auto value = std::tuple{
					Register<&T::OnAdd, Trigger::Add>
				};
			};
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
			return nox::detail::NodeDescriptorBuilder<T, RegisterList<T>::value>::value;
		}
	};
}