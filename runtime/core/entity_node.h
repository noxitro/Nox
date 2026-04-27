// Copyright (C) 2026 NOX ENGINE All rights reserved.

/// @file	entity_node.h
/// @brief	entity_node
#pragma once
#include	"node.h"
#include	"attribute_common.h"

namespace nox
{
	class Component;
	class Transform;

	class EntityNode final : public nox::Node
	{
		NOX_DECLARE_MANAGED_OBJECT(EntityNode, nox::Node);
	private:

	public:
		/*	using Collection = nox::Collection < nox::Component,
				+[](nox::Component& component) { return component.chain_; },
				+[](nox::Component& component) { return &component; }
			> ;*/


	public:
		EntityNode();
		~EntityNode()override;

		/// @brief	
		/// @param name 
		/// @param pos 
		/// @param rotation 
		/// @return 
		[[nodiscard]] static nox::IntrusivePtr<EntityNode> Create(nox::U8StringView name, const nox::Vec3& pos = nox::Vec3::Zero(), const nox::Quat& rotation = nox::Quat::Identity());
		static void Destroy(EntityNode& gameObject);

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