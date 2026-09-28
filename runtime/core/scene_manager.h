//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	scene_manager.h
///	@brief	scene_manager
#pragma once
#include	"service.h"

namespace nox
{
	class SceneView;

	/// @brief		メインのシーンビュー(メインウィンドウ)を持つService。
	/// @details	OnInitialize でメインウィンドウを作り(studio mode でなければ表示する)、OnShutdown で破棄する。
	///				OnInitialize は World::Init の中、UI スレッドのメッセージループに入る前に呼ばれる。
	///				ウィンドウの生成は UI スレッド上でその場で行われる(nox::os::detail::DispatchCreateNativeWindow)。
	///				OnShutdown はゲームスレッドが止まりメッセージループを抜けた後(World::Exit)に呼ばれる。
	///
	///				毎フレームの処理(ノード)は持たない。旧 Update フェーズが行っていた --exit-after-frames の終了要求は
	///				World::Update が判定し、RequestCloseMainWindow を呼ぶ(nox::ShouldRequestExitAfterFrames)。
	///				studio mode の判定は World を介さず nox::ResolveStudioMode でコマンドラインから行う
	///				(Service は World への参照を持たない)。
	///
	///				他のノードから使うときは引数で受け取る。エンジン内部の Editor 向けクエリの実装など、
	///				引数で受け取れない箇所は nox::World::TryGetService で引く。
	class SceneManager final : public nox::Service
	{
		NOX_DECLARE_OBJECT(SceneManager, nox::Service);
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
		/// @brief メインウィンドウを作り、studio mode でなければ表示する。
		/// @return ウィンドウを作れなければ false (起動を止める)。ウィンドウが無いと、閉じる経路で終了できない。
		bool	OnInitialize(nox::ServiceContext& context)noexcept override;

		/// @brief メインウィンドウを破棄する。
		void	OnShutdown()noexcept override;

	private:
		nox::SceneView* main_scene_view_;

		nox::Vector<std::reference_wrapper<nox::SceneView>> scene_view_list_;
	};
}