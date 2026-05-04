//	Copyright (c) 2025 NOX ENGINE All rights reserved.

///	@file	scene_manager.cpp
///	@brief	scene_manager
#include	"pch.h"
#include	"scene_manager.h"

#include	"application.h"
#include	"scene_node.h"
#include	"scene_view.h"

nox::SceneManager::SceneManager()noexcept:
	main_scene_(nullptr),
	main_scene_view_(nullptr)
{

}

nox::SceneManager::~SceneManager()
{

}

void	nox::SceneManager::Initialize(nox::Application& application)
{
	main_scene_ = new SceneNode();

	//	windowを生成
	{
		nox::os::WindowSetupDesc desc;
		desc.width = 1280;
		desc.height = 720;
		desc.window_style = nox::os::WindowStyle::Normal;
		desc.title_ptr = u"runtime";

		main_scene_view_ = new nox::SceneView();
		main_scene_view_->MakeWindow(desc);

		//	studio modeならウィンドウを表示しない
		if (!application.IsStudioMode())
		{
			main_scene_view_->GetWindow().Show();
		}
	}
}

void	nox::SceneManager::Update(nox::Application& application)
{

}

void	nox::SceneManager::Finalize(nox::Application& application)
{
	nox::util::SafeDelete(main_scene_);
	nox::util::SafeDelete(main_scene_view_);
}

std::span<const nox::EngineSystem::PhaseRegister> nox::SceneManager::GetPhaseRegisterList()const noexcept
{
	static constexpr auto table = {
		PhaseRegister(k_phase_init),
		PhaseRegister(k_phase_update),
		PhaseRegister(k_phase_terminal)
	};
	return table;
}