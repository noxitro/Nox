//	Copyright (C) 2026 NOX ENGINE All rights reserved.

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
		void CreateEngineSystems(nox::PmrVector<nox::SystemBase*>& out)const override;
	};
}