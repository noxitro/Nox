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

	public:
		EntityNode();
		~EntityNode()override;

		[[nodiscard]] static nox::IntrusivePtr<EntityNode> Create(nox::U8StringView name, const nox::Position& pos = {}, const nox::Quat& rotation = nox::Quat::Identity());
		static void Destroy(EntityNode& entity_node);

		inline void SetName(nox::U8StringView name) { name_ = name; }
		inline nox::U8StringView GetName()const noexcept { return name_; }

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

		nox::Component* CreateComponent(const nox::reflection::Type& type);

		template<std::derived_from<nox::Component> T> requires(std::is_abstract_v<T> == false)
			inline T* CreateComponent()
		{
			return static_cast<T*>(this->CreateComponent(nox::reflection::Typeof<T>()));
		}

		inline nox::Transform& GetTransform()const noexcept { return nox::util::Deref(transform_); }
		void EnumComponents(std::function<void(nox::Component&)> func)const;
	private:
		nox::Transform* transform_;

		nox::Vector<std::reference_wrapper<nox::Component>> component_list_;

		// @brief 名前
		NOX_ATTR_DECLARE(nox::attr::DataMember())
		nox::U8String name_;

		mutable nox::os::ReadWriteLock component_list_lock_;
	};
}
