//	Copyright (C) 2023 NOX ENGINE All Rights Rserved.

///	@file	component.cpp
///	@brief	component
#include	"stdafx.h"
#include	"component.h"

#include	"game_object.h"

void	nox::Component::SetOwner(nox::GameObject& owner)noexcept
{
	NOX_ASSERT(owner_ == nullptr, U"既にGameObjectをアタッチ済みです");
	owner_ = &owner;
}
