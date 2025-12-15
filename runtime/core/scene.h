//	Copyright (C) 2024 NOX ENGINE All Rights Rserved.

///	@file	scene.h
///	@brief	scene
#pragma once
#include	"managed_object.h"

namespace nox
{
	class GameObject;

	class Scene : public nox::ManagedObject
	{
		NOX_DECLARE_OBJECT(nox::Scene, nox::ManagedObject);
	public:
		void	SetResource(class SceneResource& resource);

	private:

	private:
		nox::Vector<std::reference_wrapper<GameObject>> object_list_;
	};
}