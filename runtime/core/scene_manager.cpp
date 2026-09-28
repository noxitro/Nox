//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	scene_manager.cpp
///	@brief	scene_manager
#include	"pch.h"
#include	"scene_manager.h"

#include	"world.h"
#include	"scene_view.h"

nox::SceneManager::SceneManager()noexcept:
	main_scene_view_(nullptr),
	close_requested_(false)
{

}

nox::SceneManager::~SceneManager()
{

}

void	nox::SceneManager::Initialize(nox::World& world)
{

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
		if (!world.IsStudioMode())
		{
			main_scene_view_->GetWindow().Show();
		}
	}
}

void	nox::SceneManager::Update(nox::World& world)
{
	//	--exit-after-frames=N (CI のスモーク実行用)。N フレーム目にメインウィンドウを閉じる。
	//	ユーザーがウィンドウを閉じたときと同じ経路 (WM_CLOSE → WM_DESTROY → WM_QUIT) で終わるので、
	//	Terminate フェーズから reflection / memory の終了処理まで、普段の終了と同じ順に通る。
	//	フレーム数は World::Update がこのフェーズの後に数えるので、N 回目の呼び出しでは N - 1 になっている。
	const nox::uint32 exit_after_frames = world.GetExitAfterFrames();
	if ((exit_after_frames != 0u) && (close_requested_ == false) && ((world.GetFrameCount() + 1u) >= exit_after_frames))
	{
		if (main_scene_view_ != nullptr)
		{
			main_scene_view_->GetWindow().RequestClose();
		}
		close_requested_ = true;
	}
}

void	nox::SceneManager::Finalize([[maybe_unused]] nox::World& world)
{
	nox::util::SafeDelete(main_scene_view_);
}

std::span<const nox::SystemBase::PhaseRegister> nox::SceneManager::GetPhaseRegisterList()const noexcept
{
	static constexpr auto table = {
		PhaseRegister(k_phase_init),
		PhaseRegister(k_phase_update),
		PhaseRegister(k_phase_terminal)
	};
	return table;
}
