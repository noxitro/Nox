//	Copyright (C) 2024 NOX ENGINE All Rights Rserved.

///	@file	core_entry.cpp
///	@brief	core_entry
#include	"stdafx.h"
#include	"core_entry.h"

#include	"test/test.h"

nox::CoreEntry::CoreEntry()
{
}

nox::CoreEntry::~CoreEntry()
{
}

void	nox::CoreEntry::Entry()
{
	Register(&nox::CoreEntry::Init, ModuleEntryCategory::CoreInit);
}

void	nox::CoreEntry::Init()
{
	nox::test::Test();
}