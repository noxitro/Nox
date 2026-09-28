//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	scene_manager.h
///	@brief	scene_manager
#pragma once
#include	"system.h"

namespace nox
{
	class SceneView;

	class SceneManager : public nox::SystemBase
	{
		NOX_DECLARE_OBJECT(SceneManager, nox::SystemBase);
	public:
		SceneManager()noexcept;
		~SceneManager()override;

		inline nox::SceneView& GetMainSceneView() noexcept { return *main_scene_view_; }
		inline const nox::SceneView& GetMainSceneView()const noexcept { return *main_scene_view_; }

		/// @brief メインウィンドウを閉じるよう要求する。ユーザーが閉じるボタンを押したのと同じ経路で終了する。
		/// @details WM_CLOSE を投げるだけ (nox::os::Window::RequestClose) なので、どのスレッドから呼んでもよい。
		///          ウィンドウがまだ無い、または既に閉じていれば何もしない。
		///          --exit-after-frames の終了要求 (World::Update) が使う。
		void	RequestCloseMainWindow()noexcept;

	private:
		void	Initialize(nox::World& world);
		void	Finalize(nox::World& world);

		std::span<const nox::SystemBase::PhaseRegister> GetPhaseRegisterList()const noexcept override;
	public:
		static constexpr SystemPhaseInit k_phase_init{
			&SceneManager::Initialize,
			u8"SceneManager::Initialize"
		};

		static constexpr SystemPhaseTerminate k_phase_terminal{
			&SceneManager::Finalize,
			u8"SceneManager::Finalize"
		};

	private:
		nox::SceneView* main_scene_view_;

		nox::Vector<std::reference_wrapper<nox::SceneView>> scene_view_list_;
	};
}