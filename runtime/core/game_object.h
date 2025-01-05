//	Copyright (C) 2023 NOX ENGINE All Rights Rserved.

///	@file	game_object.h
///	@brief	game_object
#pragma once

#include	"managed_object.h"
#include	"attribute_common.h"

namespace nox
{
	class Component;
	class Transform;

	class GameObject final: public ManagedObject
	{
		NOX_DECLARE_MANAGED_OBJECT(GameObject, ManagedObject);
	private:

	public:
	/*	using Collection = nox::Collection < nox::Component,
			+[](nox::Component& component) { return component.chain_; },
			+[](nox::Component& component) { return &component; }
		> ;*/


	public:
		GameObject();
		~GameObject()override;

		/// @brief	
		/// @param name 
		/// @param pos 
		/// @param rotation 
		/// @return 
		[[nodiscard]]	static nox::IntrusivePtr<GameObject> Create(nox::StringView name, const nox::Vec3& pos = nox::Vec3::Zero(), const nox::Quat& rotation = nox::Quat::Identity());
		static void Destroy(GameObject& gameObject);

		[[nodiscard]] Component* GetComponent(const nox::reflection::Type& type)noexcept;
		[[nodiscard]] Component* GetSameComponent(const nox::reflection::Type& type)noexcept;

		template<std::derived_from<Component> T> requires(std::is_final_v<T> == false)
		inline T* GetComponent()noexcept
		{
			return GetComponent(nox::reflection::Typeof<T>());
		}

		template<std::derived_from<Component> T>
		inline T* GetSameComponent()noexcept
		{
			return GetComponent(nox::reflection::Typeof<T>());
		}

		/// @brief 
		/// @param type 
		/// @return 
		IntrusivePtr<Component> CreateComponent(const nox::reflection::Type& type);

		/// @brief 
		/// @tparam T 
		/// @return 
		template<std::derived_from<Component> T> requires(std::is_abstract_v<T> == false)
		inline IntrusivePtr<T> CreateComponent()
		{
			return nox::IntrusivePtrDynamicCast<T>(std::move(CreateComponent(nox::reflection::Typeof<T>())));
		}

		inline nox::Transform& Transform()const noexcept { return nox::util::Deref(transform_); }
	private:
		/// @brief 
		///	@detail	必ず持っているコンポーネント
		class nox::Transform* transform_;

		// @brief 名前
		nox::String name_;
	};
}