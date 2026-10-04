// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

/// @file	debug_draw.h
/// @brief	debug_draw
#pragma once

#if NOX_DEVELOP

namespace nox::render::debug
{
	enum class DebugDrawOption : nox::uint32
	{
		None,
		ZTest,
		ZWrite,
	};

	class DebugDraw : public nox::SystemBase
	{
		NOX_DECLARE_OBJECT(nox::render::debug::DebugDraw, nox::SystemBase);
	public:

		void DrawLine(const nox::Float3& start, const nox::Float3& end, nox::Color color, DebugDrawOption option = DebugDrawOption::None);

		std::span<const nox::SystemBase::PhaseRegister> GetPhaseRegisterList()const noexcept override;

	private:
		void Init(nox::World& world);
		void UpdateDraw(nox::World& world);
		void Terminate(nox::World& world);

	public:
		static constexpr SystemPhaseInit kPhaseInit{ &DebugDraw::Init, u8"DebugDraw::Initialize" };
		static constexpr SystemPhaseUpdate kPhaseUpdate{ &DebugDraw::UpdateDraw, u8"DebugDraw::Update" };
		static constexpr SystemPhaseTerminate kPhaseTerminate{ &DebugDraw::Terminate, u8"DebugDraw::Terminate" };


	};
}

#endif // NOX_DEVELOP