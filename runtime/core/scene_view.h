//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	scene_view.h
///	@brief	scene_view
#pragma once
#include	"object.h"

namespace nox
{
	/// @brief		シーンビュー
	/// @details	シーン情報とウィンドウを管理する
	class SceneView : public nox::Object
	{
		NOX_DECLARE_OBJECT(nox::SceneView, nox::Object);
	public:
		SceneView()noexcept;
		~SceneView()override;

		inline constexpr nox::Window& GetWindow() noexcept { return window_; }
		inline constexpr const nox::Window& GetWindow()const noexcept { return window_; }

		void MakeWindow(const nox::WindowSetupDesc& window_desc);
	private:
		nox::Window window_;
	};
}
