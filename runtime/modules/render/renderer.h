//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	renderer.h
///	@brief	renderer
#pragma once
#include	"../../core/service.h"
#include	"../../core/service_attribute.h"

namespace nox::render
{
	class RenderDevice;

	/// @brief		描画デバイスを持つService。
	/// @details	デバイスはコンストラクタで作り、デストラクタで破棄する。OnInitialize / OnShutdown は今は何もしない。
	///				毎フレームの処理は Presentation の Update(描画の抽出・提出の置き場所。今は空)。
	///				他のServiceとの依存は持たない。描画に依存するService(nox::render::debug::DebugDraw など)が
	///				Depends / RunAfter でこの型に並ぶ。
	class Renderer final : public nox::Service
	{
		NOX_DECLARE_OBJECT(nox::render::Renderer, nox::Service);
	public:
		Renderer();
		~Renderer()override;

	private:
		bool OnInitialize(nox::ServiceContext& context)noexcept override;
		void OnShutdown()noexcept override;

		/// @brief 描画の抽出・提出。Presentation に1フレーム1回。今は空。
		NOX_ATTR(nox::attr::ServiceMethod(nox::SystemPhaseType::Presentation))
		void Update();

		inline constexpr auto& GetDevice()const noexcept;

	private:
		nox::render::RenderDevice* render_device_;
	};
}