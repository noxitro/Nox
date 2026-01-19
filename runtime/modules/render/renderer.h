//	Copyright (C) 2026 NOX ENGINE All rights reserved.

///	@file	renderer.h
///	@brief	renderer
#pragma once

namespace nox::render
{
	class RenderDevice;

	class Renderer : public nox::Object, public nox::ISingleton<Renderer>
	{
		NOX_DECLARE_OBJECT(nox::render::Renderer, nox::Object);
	public:
		Renderer();
		~Renderer()override;

	private:
		inline constexpr auto& GetDevice()const noexcept;

	private:
		class nox::render::RenderDevice* render_device_;
	};
}