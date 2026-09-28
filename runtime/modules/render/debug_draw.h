// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

/// @file	debug_draw.h
/// @brief	debug_draw
#pragma once

#if NOX_DEVELOP
#include	"../../core/service.h"
#include	"../../core/service_attribute.h"
#include	"renderer.h"

namespace nox::render::debug
{
	enum class DebugDrawOption : nox::uint32
	{
		None,
		ZTest,
		ZWrite,
	};

	/// @brief		開発用のデバッグ描画を受け付けるService(開発ビルドのみ)。
	/// @details	nox::render::Renderer に描画を渡すので、Depends で Renderer を先に初期化し、後に終了させる。
	///				毎フレームの処理は Presentation の UpdateDraw(今は空)。RunAfter で Renderer の Presentation の後に走る。
	class DebugDraw final : public nox::Service
	{
		NOX_DECLARE_OBJECT(nox::render::debug::DebugDraw, nox::Service);
	public:
		/// @brief 描画を渡す先。初期化はこちらが後、終了はこちらが先。
		using Depends = nox::TypeList<nox::render::Renderer>;
		/// @brief Renderer の Presentation の後に走る。
		using RunAfter = nox::TypeList<nox::render::Renderer>;

		void DrawLine(const nox::Float3& start, const nox::Float3& end, nox::Color color, DebugDrawOption option = DebugDrawOption::None);

	private:
		/// @brief Depends に並べた Renderer を受け取る(引けなければ起動失敗)。
		bool OnInitialize(nox::ServiceContext& context)noexcept override;
		void OnShutdown()noexcept override;

		/// @brief 受け付けたデバッグ描画を Renderer へ渡す。Presentation に1フレーム1回。今は空。
		NOX_ATTR(nox::attr::ServiceMethod(nox::SystemPhaseType::Presentation))
		void UpdateDraw();

	private:
		nox::render::Renderer* renderer_ = nullptr;
	};
}

#endif // NOX_DEVELOP