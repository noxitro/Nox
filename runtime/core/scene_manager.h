//	Copyright (C) 2025 NOX ENGINE All rights reserved.

///	@file	scene_manager.h
///	@brief	scene_manager
#pragma once
#include	"engine_system.h"

namespace nox
{
	class SceneNode;
	class SceneView;
	class Application;

	class SceneManager : public nox::EngineSystem
	{
		NOX_DECLARE_OBJECT(SceneManager, nox::EngineSystem);
	public:
		SceneManager()noexcept;
		~SceneManager()override;

		inline nox::SceneNode& GetMainScene() noexcept { return *main_scene_; }
		inline nox::SceneView& GetMainSceneView() noexcept { return *main_scene_view_; }
		inline const nox::SceneView& GetMainSceneView()const noexcept { return *main_scene_view_; }

	private:
		void	Initialize(nox::Application& application);
		void	Update(nox::Application& application);
		void	Finalize(nox::Application& application);

		std::span<const nox::EngineSystem::PhaseRegister> GetPhaseRegisterList()const noexcept override;
	public:
		static constexpr SystemPhaseInit k_phase_init{
			&SceneManager::Initialize,
			u8"SceneManager::Initialize"
		};

		static constexpr SystemPhaseUpdate k_phase_update{
			&SceneManager::Update,
			u8"SceneManager::Update"
		};

		static constexpr SystemPhaseTerminate k_phase_terminal{
			&SceneManager::Finalize,
			u8"SceneManager::Finalize"
		};

	private:
		nox::SceneNode* main_scene_;
		nox::SceneView* main_scene_view_;

		nox::Vector<std::reference_wrapper<nox::SceneView>> scene_view_list_;
	};
}