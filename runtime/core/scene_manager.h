//	Copyright (C) 2025 NOX ENGINE All rights reserved.

///	@file	scene_manager.h
///	@brief	scene_manager
#pragma once
#include	"object.h"

namespace nox
{
	class Scene;
	class SceneView;

	class SceneManager : public nox::Object, public nox::ISingleton<SceneManager>
	{
		NOX_DECLARE_OBJECT(SceneManager, nox::ISingleton<SceneManager>);
	public:
		SceneManager()noexcept;
		~SceneManager()override;

		void	Initialize(nox::U8StringView main_scene_path);
		void	Update();
		void	Finalize();

		inline nox::Scene& GetMainScene() noexcept { return *main_scene_; }
		inline nox::SceneView& GetMainSceneView() noexcept { return *main_scene_view_; }

	private:
		nox::Scene* main_scene_;
		nox::SceneView* main_scene_view_;

		nox::Vector<std::reference_wrapper<nox::SceneView>> scene_view_list_;
	};
}