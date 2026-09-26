//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	renderer.h
///	@brief	renderer
#pragma once

namespace nox::render
{
	class RenderDevice;

	class Renderer : public nox::SystemBase
	{
		NOX_DECLARE_OBJECT(nox::render::Renderer, nox::Object);
	public:
		Renderer();
		~Renderer()override;

		void Init(nox::World& world);
		void Update(nox::World& world);
		void Terminate(nox::World& world);

	public:
		static constexpr nox::SystemBase::SystemPhaseInit kPhaseInit{ &Renderer::Init, u8"Renderer::Init" };
		static constexpr nox::SystemBase::SystemPhaseUpdate kPhaseUpdate{ &Renderer::Update, u8"Renderer::Update" };
		static constexpr nox::SystemBase::SystemPhaseTerminate kPhaseTerminate{ &Renderer::Terminate, u8"Renderer::Terminate" };

	private:
		std::span<const nox::SystemBase::PhaseRegister> GetPhaseRegisterList()const noexcept override;

		inline constexpr auto& GetDevice()const noexcept;

	private:
		nox::render::RenderDevice* render_device_;
	};
}