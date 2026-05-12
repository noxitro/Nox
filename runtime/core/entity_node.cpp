// Copyright (C) 2026 NOX ENGINE All rights reserved.

/// @file	entity_node.cpp
/// @brief	entity_node
#include "pch.h"
#include "entity_node.h"

#include	"application.h"
#include	"component.h"
#include	"dev/editor_remote_server.h"
#include	"scene_manager.h"
#include	"scene_node.h"
#include	"transform.h"

namespace nox
{
}

nox::EntityNode::EntityNode() :
	transform_(nullptr),
	name_{}
{
}

nox::EntityNode::~EntityNode()
{
	for (nox::Component& component : component_list_)
	{
		component.UnLoaded();
		component.ReleaseRef();
	}
	transform_ = nullptr;
}

nox::IntrusivePtr<nox::EntityNode> nox::EntityNode::Create(nox::U8StringView name, const nox::Position& pos, const nox::Quat& rotation)
{
	nox::EntityNode* const entity_node = new nox::EntityNode();
	entity_node->AddRef();
	entity_node->name_ = name;

	constexpr auto nse = std::derived_from< nox::Transform, nox::Component>;
	entity_node->transform_ = entity_node->CreateComponent<nox::Transform>();

	return nox::IntrusivePtr<nox::EntityNode>(entity_node);
}

void nox::EntityNode::Destroy(nox::EntityNode& entity_node)
{
#if NOX_DEVELOP
	if (nox::Application* const application = entity_node.GetApplicationPtr())
	{
		nox::SceneManager* const scene_manager = application->FindSystem<nox::SceneManager>();
		if (scene_manager != nullptr)
		{
			scene_manager->GetMainScene().RemoveEntity(entity_node);
		}

		nox::dev::editor_remote::EditorRemoteServerSystem* const remote_system = application->FindSystem<nox::dev::editor_remote::EditorRemoteServerSystem>();
		if (remote_system != nullptr)
		{
			nox::dev::editor_remote::EditorRemoteServer& server = remote_system->GetServer();

#if NOX_DEVELOP
			for (nox::Component& comp : entity_node.component_list_)
			{
				server.NotifyRemoteInstanceDestroyed(comp);
			}
#endif // NOX_DEVELOP

			if (server.NotifyRemoteInstanceDestroyed(entity_node))
			{
				return;
			}
		}
	}
#endif
	entity_node.ReleaseRef();
}

nox::Component* nox::EntityNode::GetComponent(const nox::reflection::Type& type)const noexcept
{
	NOX_LOCAL_SCOPE(nox::os::ScopedReadLock(component_list_lock_));

	const nox::reflection::ClassInfo* const class_info = type.GetUserDefinedCompoundTypeInfo();
	if (class_info == nullptr)
	{
		return nullptr;
	}

	for (Component& component : component_list_)
	{
		if (component.GetType().GetUserDefinedCompoundTypeInfo()->IsBaseOf(*class_info) == true)
		{
			return &component;
		}
	}
	return nullptr;
}

nox::Component* nox::EntityNode::GetSameComponent(const nox::reflection::Type& type)const noexcept
{
	NOX_LOCAL_SCOPE(nox::os::ScopedReadLock(component_list_lock_));

	for (Component& component : component_list_)
	{
		if (component.GetType() == type)
		{
			return &component;
		}
	}
	return nullptr;
}

nox::Component* nox::EntityNode::CreateComponent(const nox::reflection::Type& type)
{
	nox::Object* object = nullptr;
	nox::Component* component = nullptr;
	if (type == nox::reflection::Typeof<nox::Transform>())
	{
		nox::Transform* const transform = new nox::Transform();
		object = transform;
		component = transform;
	}
	else
	{
		const nox::reflection::ClassInfo* const class_info = type.GetUserDefinedCompoundTypeInfo();
		if (class_info == nullptr || class_info->IsSubclassOf<nox::Component>() == false)
		{
			return nullptr;
		}

		object = static_cast<nox::Object*>(type.CreateObject());
		component = nox::reflection::AsCast<nox::Component*>(object);
	}

	if (component == nullptr)
	{
		delete object;
		return nullptr;
	}

	component->SetOwner(*this);
	component->AddRef();

	if (transform_ == nullptr)
	{
		transform_ = static_cast<nox::Transform*>(component);
	}

	{
		NOX_LOCAL_SCOPE(nox::os::ScopedWriteLock(component_list_lock_));
		component_list_.emplace_back(*component);
	}

	component->Loaded();

	return component;
}

void nox::EntityNode::EnumComponents(std::function<void(nox::Component&)> func)const
{
	NOX_LOCAL_SCOPE(nox::os::ScopedReadLock(component_list_lock_));

	for (Component& component : component_list_)
	{
		func(component);
	}
}