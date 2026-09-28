//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	scene_manager.cpp
///	@brief	scene_manager
#include	"pch.h"
#include	"scene_manager.h"

#include	"world.h"
#include	"scene_view.h"
#include	"log_id.h"

nox::SceneManager::SceneManager()noexcept:
	main_scene_view_(nullptr)
{

}

nox::SceneManager::~SceneManager()
{
	//	OnShutdown を通らなかった場合(起動途中の失敗など)も解放する。通っていれば何もしない。
	nox::util::SafeDelete(main_scene_view_);
}

bool	nox::SceneManager::OnInitialize([[maybe_unused]] nox::ServiceContext& context)noexcept
{
	//	windowを生成
	nox::os::WindowSetupDesc desc;
	desc.width = 1280;
	desc.height = 720;
	desc.window_style = nox::os::WindowStyle::Normal;
	desc.title_ptr = u"runtime";

	main_scene_view_ = new nox::SceneView();
	main_scene_view_->MakeWindow(desc);

	if (main_scene_view_->GetWindow().GetNativeHandle() == nullptr)
	{
		//	ウィンドウが無いと、ユーザーの操作でも --exit-after-frames でも閉じる経路で終了できない。
		NOX_ERROR_LINE(nox::log_id::CoreCommon, u8"メインウィンドウの生成に失敗しました");
		nox::util::SafeDelete(main_scene_view_);
		return false;
	}

	//	studio modeならウィンドウを表示しない。
	//	World を介さず、World と同じ規則でコマンドラインから決める(Service は World への参照を持たない)。
	if (nox::ResolveStudioMode(nox::os::GetCommandLineArgList()) == false)
	{
		main_scene_view_->GetWindow().Show();
	}
	return true;
}

void	nox::SceneManager::OnShutdown()noexcept
{
	nox::util::SafeDelete(main_scene_view_);
}

void	nox::SceneManager::RequestCloseMainWindow()noexcept
{
	if (main_scene_view_ != nullptr)
	{
		main_scene_view_->GetWindow().RequestClose();
	}
}
