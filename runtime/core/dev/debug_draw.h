// Copyright (C) 2026 NOX ENGINE All rights reserved.

/// @file	dev_graphic.h
/// @brief	開発用デバッグ描画
#pragma once
#include	"../system.h"

namespace nox::dev
{
	class DebugDraw : public nox::SystemBase
	{
		NOX_DECLARE_OBJECT(nox::dev::DebugDraw, nox::SystemBase);
	public:

	private:

		void Init(nox::World& world);
		void Update(nox::World& world);
		void Terminate(nox::World& world);
		std::span<const nox::SystemBase::PhaseRegister> GetPhaseRegisterList()const noexcept override;
	public:
//		void DrawPoint(const nox::Float3& position, const nox::Color color, nox::float_t size = 1.0f);

	public:
		static constexpr nox::SystemBase::SystemPhaseInit kPhaseInit{ &DebugDraw::Init, u8"DebugDraw::Init" };
		static constexpr nox::SystemBase::SystemPhaseUpdate kPhaseUpdate{ &DebugDraw::Update, u8"DebugDraw::Update" };
		static constexpr nox::SystemBase::SystemPhaseTerminate kPhaseTerminate{ &DebugDraw::Terminate, u8"DebugDraw::Terminate" };
	};
}