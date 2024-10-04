//	Copyright (C) 2023 NOX ENGINE All Rights Rserved.

///	@file	game_object.cpp
///	@brief	game_object
#include	"stdafx.h"
#include	"game_object.h"

#include	"component.h"
#include	"transform.h"

nox::GameObject::GameObject()
{

}

nox::GameObject::~GameObject()
{

}

nox::IntrusivePtr<nox::Component> nox::GameObject::GetComponent(const nox::reflection::Type& type)noexcept
{
	for (nox::Component* component : components_)
	{
		if (component->GetType() == type)
		{
			return nox::IntrusivePtr(component);
		}
	}

	return nox::IntrusivePtr<Component>();
}

nox::IntrusivePtr<nox::Component> nox::GameObject::GetSameComponent(const nox::reflection::Type& type)noexcept
{
	return nox::IntrusivePtr<Component>();
}

nox::IntrusivePtr<nox::Component> nox::GameObject::CreateComponent(const nox::reflection::Type& type)
{
	return nox::IntrusivePtr<Component>();
}