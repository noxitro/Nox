// Copyright (C) 2026 NOX ENGINE All rights reserved.

/// @file	scene_node.cpp
/// @brief	scene_node
#include "pch.h"
#include "scene_node.h"

#include	"entity_node.h"
#include	"scene_resource.h"

void	nox::SceneNode::SetResource(SceneResource& resource)
{
}

void nox::SceneNode::AddEntity(nox::EntityNode& entity_node, nox::Node* parent)
{
	for (const std::reference_wrapper<EntityNode>& registered_entity : object_list_)
	{
		if (&registered_entity.get() == &entity_node)
		{
			entity_node.SetParent(parent != nullptr ? parent : this);
			return;
		}
	}

	entity_node.SetParent(parent != nullptr ? parent : this);
	object_list_.emplace_back(entity_node);
}

bool nox::SceneNode::RemoveEntity(nox::EntityNode& entity_node)noexcept
{
	for (auto it = object_list_.begin(); it != object_list_.end(); ++it)
	{
		if (&it->get() == &entity_node)
		{
			entity_node.SetParent(nullptr);
			object_list_.erase(it);
			return true;
		}
	}

	return false;
}
