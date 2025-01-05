//	Copyright (C) 2024 NOX ENGINE All Rights Rserved.

///	@file	module_entry.cpp
///	@brief	module_entry
#include	"stdafx.h"
#include	"module_entry.h"

#include	"application.h"

void nox::ModuleEntry::RegisterImpl(void(*func)(nox::ModuleEntry&), const nox::ModuleEntryCategory type)
{
	Application::Instance().RegisterModuleEntry(func, *this, type);
}