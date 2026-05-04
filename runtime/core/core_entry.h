//	Copyright (C) 2024 NOX ENGINE All Rights Rserved.

///	@file	core_entry.h
///	@brief	core_entry
#pragma once

#include	"module_entry.h"

namespace nox
{
	class CoreModule : public nox::EngineModule
	{
		NOX_DECLARE_OBJECT(nox::CoreModule, nox::EngineModule);
	public:
		CoreModule();
		~CoreModule()override;

		void CreateEngineSystems(nox::PmrVector<nox::EngineSystem*>& out)const override;
	public:
	};
}