//	Copyright (C) 2023 NOX ENGINE All Rights Rserved.

///	@file	scene_view.h
///	@brief	scene_view
#pragma once
#include	"managed_object.h"
namespace nox
{
	class SceneView : public nox::ManagedObject
	{
		NOX_DECLARE_OBJECT(nox::SceneView, nox::ManagedObject);
	public:
		SceneView()noexcept;
		~SceneView()override;
	private:

		nox::IntrusivePtr<class Scene> scene_;
	};
}