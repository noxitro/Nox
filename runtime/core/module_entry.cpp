//	Copyright (C) 2024 NOX ENGINE All Rights Rserved.

///	@file	module_entry.cpp
///	@brief	module_entry
#include	"stdafx.h"
#include	"module_entry.h"

#include	"application.h"

void nox::ModuleEntry::Register(void(ModuleEntry::* func)(), const nox::ModuleEntryCategory type)
{
	Application::Instance().RegisterModuleEntry(func, *this, type);
}