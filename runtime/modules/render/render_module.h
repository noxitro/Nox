//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	render_module.h
///	@brief	render_module
#pragma once

namespace nox::render
{
	class RenderModule : public nox::EngineModule
	{
		NOX_DECLARE_OBJECT(nox::render::RenderModule, nox::EngineModule);
	public:
		RenderModule();
		~RenderModule()noexcept = default;

	private:
		/// @brief 描画のServiceを登録する(Renderer。開発ビルドでは DebugDraw も)。
		void RegisterServices(nox::World& world)const override;
	};
}