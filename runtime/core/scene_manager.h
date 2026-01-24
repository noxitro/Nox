//	Copyright (C) 2025 NOX ENGINE All rights reserved.

///	@file	scene_manager.h
///	@brief	scene_manager
#pragma once
#include	"object.h"

namespace nox
{
	class Scene;

	class SceneManager : public nox::Object, public nox::ISingleton<SceneManager>
	{
		NOX_DECLARE_OBJECT(SceneManager, nox::ISingleton<SceneManager>);
	public:
		void	Initialize(nox::U8StringView main_scene_path);
		void	Update();
		void	Finalize();

		inline Scene& GetMainScene() noexcept { return *main_scene_; }

	private:
		Scene* main_scene_;
	};
}