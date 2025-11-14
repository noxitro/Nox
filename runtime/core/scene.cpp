//	Copyright (C) 2024 NOX ENGINE All Rights Rserved.

///	@file	scene.cpp
///	@brief	scene
#include	"stdafx.h"
#include	"scene.h"

#include	"scene_resource.h"

void	nox::Scene::setResource(SceneResource& resource)
{
	using t = ToMemberFunctionPointerType<nox::String()const, nox::Object>;
	static_cast<ToMemberFunctionPointerType<nox::String()const, nox::Object>>(&nox::Object::ToString);
}