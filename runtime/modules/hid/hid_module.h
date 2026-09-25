//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	hid_module.h
///	@brief	hid module
#pragma once

namespace nox::hid
{
	class Module : public nox::EngineModule
	{
		NOX_DECLARE_OBJECT(nox::hid::Module, nox::EngineModule);
	public:
		Module();
		~Module()noexcept = default;

	private:
		void CreateEngineSystems(nox::PmrVector<nox::SystemBase*>& out)const override;
	};
}
