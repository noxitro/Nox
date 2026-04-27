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

	private:

	private:
		nox::Vector<std::reference_wrapper<EntityNode>> object_list_;
	};
}