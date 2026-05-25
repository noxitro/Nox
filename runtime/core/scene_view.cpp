//	Copyright (C) 2023 NOX ENGINE All Rights Rserved.

///	@file	scene_view.cpp
///	@brief	scene_view
#include	"pch.h"
#include	"scene_view.h"

nox::SceneView::SceneView()noexcept
{

}

nox::SceneView::~SceneView()
{
}

void nox::SceneView::MakeWindow(const nox::os::WindowSetupDesc& window_desc)
{
	window_.Create(window_desc);
}