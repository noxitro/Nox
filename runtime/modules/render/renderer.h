//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	renderer.h
///	@brief	renderer
#pragma once

namespace nox::render
{
	class RenderDevice;

	class Renderer final : public nox::Service<Renderer>
	{
		NOX_ECS_DECLARE_VERIFY(Renderer);
	public:
		Renderer();
		~Renderer();

		void Init(nox::legacy::World& world);
		void Update(nox::legacy::World& world);
		void Terminate(nox::legacy::World& world);

	public:
		inline static constexpr const nox::NodeDescriptor& GetNodeDescriptor()noexcept
		{
			return node_descriptor_v<>;
		}

	private:

		inline constexpr auto& GetDevice()const noexcept;

	private:
		nox::render::RenderDevice* render_device_;
	};
}