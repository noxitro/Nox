//	Copyright (C) 2024 NOX ENGINE All Rights Rserved.

///	@file	core_entry.h
///	@brief	core_entry
#pragma once

#include	"module_entry.h"

namespace nox
{
	class CoreEntry : public nox::ModuleEntry
	{
		NOX_DECLARE_OBJECT(nox::CoreEntry, nox::ModuleEntry);
	public:
		CoreEntry();
		~CoreEntry()override;

		void Entry()override;

	private:
		void	Init();
	};
}