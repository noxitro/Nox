// Copyright (C) 2026 NOX ENGINE All rights reserved.

/// @file	scene_node.h
/// @brief	scene_node
#pragma once
#include	"node.h"

namespace nox
{
	class EntityNode;

	class SceneNode : public nox::Node
	{
		NOX_DECLARE_OBJECT(nox::SceneNode, nox::Node);
	public:
		void	SetResource(class SceneResource& resource);
		void	AddEntity(nox::EntityNode& entity_node, nox::Node* parent = nullptr);
		bool	RemoveEntity(nox::EntityNode& entity_node)noexcept;

	private:

	private:
		nox::Vector<std::reference_wrapper<EntityNode>> object_list_;
	};
}
