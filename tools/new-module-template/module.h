//	Copyright (c) 2023-{{YEAR}} noxitro
//	SPDX-License-Identifier: MIT

///	@file	{{MODULE_FILE}}.h
///	@brief	{{PROJECT}} module
#pragma once

namespace {{NAMESPACE}}
{
	class {{CLASS}} final: public nox::EngineModule
	{
		NOX_DECLARE_OBJECT({{NAMESPACE}}::{{CLASS}}, nox::EngineModule);
	public:
		{{CLASS}}();
		~{{CLASS}}()noexcept = default;

	private:
		void Register(nox::WorldRegistery& registry)const override;
	};
}
