//	Copyright (C) 2023 NOX ENGINE All Rights Rserved.

///	@file	scene_view.cpp
///	@brief	scene_view
#include	"stdafx.h"
#include	"scene_view.h"

#include	"scene.h"

nox::SceneView::SceneView(const nox::os::WindowSetupDesc& window_desc)noexcept:
	scene_(nullptr)
{
}

nox::SceneView::~SceneView()
{
}

void nox::SceneView::SetScene(Scene& scene) noexcept
{
	scene_ = &scene;
}