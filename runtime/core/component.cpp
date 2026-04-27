//	Copyright (C) 2023 NOX ENGINE All Rights Rserved.

///	@file	component.cpp
///	@brief	component
#include	"pch.h"
#include	"component.h"

#include	"entity_node.h"

void	nox::Component::SetOwner(nox::EntityNode& owner)noexcept
{
	NOX_ASSERT(owner_ == nullptr, u"既にGameObjectをアタッチ済みです");
	owner_ = &owner;
}
