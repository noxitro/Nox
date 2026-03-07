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

	class GameObject final: public nox::ManagedObject
	{
		NOX_DECLARE_MANAGED_OBJECT(GameObject, nox::ManagedObject);
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
		[[nodiscard]]	static nox::IntrusivePtr<GameObject> Create(nox::U8StringView name, const nox::Vec3& pos = nox::Vec3::Zero(), const nox::Quat& rotation = nox::Quat::Identity());
		static void Destroy(GameObject& gameObject);

		[[nodiscard]] nox::Component* GetComponent(const nox::reflection::Type& type)const noexcept;
		[[nodiscard]] nox::Component* GetSameComponent(const nox::reflection::Type& type)const noexcept;

		template<std::derived_from<nox::Component> T> requires(std::is_final_v<T> == false)
		inline T* GetComponent()const noexcept
		{
			return static_cast<T*>(GetComponent(nox::reflection::Typeof<T>()));
		}

		template<std::derived_from<nox::Component> T>
		inline T* GetSameComponent()const noexcept
		{
			return static_cast<T*>(GetSameComponent(nox::reflection::Typeof<T>()));
		}

		/// @brief 
		/// @param type 
		/// @return 
		nox::Component* CreateComponent(const nox::reflection::Type& type);

		/// @brief 
		/// @tparam T 
		/// @return 
		template<std::derived_from<nox::Component> T> requires(std::is_abstract_v<T> == false)
		inline T* CreateComponent()
		{
			return static_cast<T*>(this->CreateComponent(nox::reflection::Typeof<T>()));
		}

		inline nox::Transform& Transform()const noexcept { return nox::util::Deref(transform_); }
	private:
		/// @brief 
		///	@detail	必ず持っているコンポーネント
		class nox::Transform* transform_;

		// @brief 名前
		NOX_ATTR_DECLARE(nox::attr::DataMember())
		nox::U8String name_;
	};
}