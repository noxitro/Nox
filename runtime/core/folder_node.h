// Copyright (C) 2026 NOX ENGINE All rights reserved.

/// @file	folder_node.h
/// @brief	folder_node
#pragma once
#include	"node.h"

namespace nox
{
	class FolderNode : public Node
	{
		NOX_DECLARE_OBJECT(nox::FolderNode, nox::Node);
	public:
		FolderNode() = default;
		~FolderNode()override = default;
	};
}