//	Copyright (c) 2023-$year$ noxitro
//	SPDX-License-Identifier: MIT

///	@file	$safeprojectname$_module.h
///	@brief	$safeprojectname$ module
#pragma once

namespace nox::$safeprojectname$
{
	class Module : public nox::EngineModule
	{
		NOX_DECLARE_OBJECT(nox::$safeprojectname$::Module, nox::EngineModule);
	public:
		Module();
		~Module()noexcept = default;

	private:
		void CreateEngineSystems(nox::PmrVector<nox::SystemBase*>& out)const override;
	};
}
