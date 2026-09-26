//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	core_module.h
///	@brief	core_module
#pragma once

#include	"engine_module.h"

namespace nox
{
	class CoreModule : public nox::EngineModule
	{
		NOX_DECLARE_OBJECT(nox::CoreModule, nox::EngineModule);
	public:
		CoreModule();
		~CoreModule()override;

		void CreateEngineSystems(nox::PmrVector<nox::SystemBase*>& out)const override;
	public:
	};
}