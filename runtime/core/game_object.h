//	Copyright (C) 2023 NOX ENGINE All Rights Rserved.

///	@file	game_object.h
///	@brief	game_object
#pragma once

#include	"managed_object.h"
#include	"attribute_common.h"

namespace nox
{
	class Component;

	class GameObject final: public ManagedObject
	{
		NOX_DECLARE_MANAGED_OBJECT(GameObject, ManagedObject);
	public:
		GameObject();
		~GameObject()override;

		nox::IntrusivePtr<Component> GetComponent(const nox::reflection::Type& type)noexcept;
		nox::IntrusivePtr<Component> GetSameComponent(const nox::reflection::Type& type)noexcept;

		template<std::derived_from<Component> T> requires(std::is_final_v<T> == false)
		inline nox::IntrusivePtr<T> GetComponent()noexcept
		{
			return GetComponent(nox::reflection::Typeof<T>());
		}

		template<std::derived_from<Component> T>
		inline nox::IntrusivePtr<T> GetSameComponent()noexcept
		{
			return GetComponent(nox::reflection::Typeof<T>());
		}

		/// @brief 
		/// @param type 
		/// @return 
		nox::IntrusivePtr<Component> CreateComponent(const nox::reflection::Type& type);

		/// @brief 
		/// @tparam T 
		/// @return 
		template<std::derived_from<Component> T> requires(std::is_abstract_v<T> == false)
		inline nox::IntrusivePtr<T> CreateComponent()
		{
			return CreateComponent(nox::reflection::Typeof<T>());
		}
	private:
		nox::Vector<Component*> components_;

		// @brief 名前
		nox::String name_;
	};
}