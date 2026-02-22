//	Copyright (c) 2025 NOX ENGINE All rights reserved.

///	@file	scene_manager.cpp
///	@brief	scene_manager
#include	"stdafx.h"
#include	"scene_manager.h"

#include	"scene.h"
#include	"scene_view.h"

nox::SceneManager::SceneManager()noexcept:
	main_scene_(nullptr),
	main_scene_view_(nullptr)
{

}

nox::SceneManager::~SceneManager()
{

}

void	nox::SceneManager::Initialize(nox::U8StringView main_scene_path)
{
	main_scene_ = new Scene();

	//	windowを生成
	{
		nox::os::WindowSetupDesc desc;
		desc.width = 1280;
		desc.height = 720;
		desc.window_style = nox::os::WindowStyle::Normal;
		desc.title_ptr = u"runtime";

		main_scene_view_ = new nox::SceneView();
		main_scene_view_->MakeWindow(desc);
		//main_scene_view_->GetWindow().Show();
	}
}

void	nox::SceneManager::Update()
{

}

void	nox::SceneManager::Finalize()
{
	nox::util::SafeDelete(main_scene_);
	nox::util::SafeDelete(main_scene_view_);
}