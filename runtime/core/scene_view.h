//	Copyright (C) 2023 NOX ENGINE All Rights Rserved.

///	@file	scene_view.h
///	@brief	scene_view
#pragma once
#include	"managed_object.h"

namespace nox
{
	class Scene;

	/// @brief		シーンビュー
	/// @details	シーン情報とウィンドウを管理する
	class SceneView : public nox::ManagedObject
	{
		NOX_DECLARE_OBJECT(nox::SceneView, nox::ManagedObject);
	public:
		SceneView()noexcept;
		~SceneView()override;

		inline const nox::Scene* GetScene()const noexcept { return scene_; }
		inline constexpr nox::os::Window& GetWindow() noexcept { return window_; }
		inline constexpr const nox::os::Window& GetWindow()const noexcept { return window_; }

		void SetScene(nox::Scene& scene) noexcept;

		void MakeWindow(const nox::os::WindowSetupDesc& window_desc);
	private:
		nox::Scene* scene_;
		nox::os::Window window_;
	};
}
