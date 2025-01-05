//	Copyright (C) 2024 NOX ENGINE All Rights Rserved.

///	@file	core_entry.cpp
///	@brief	core_entry
#include	"stdafx.h"
#include	"core_entry.h"

#include	"garbage_collector.h"
#include	"test/test.h"

nox::CoreEntry::CoreEntry()
{
	Register<&nox::CoreEntry::Init>(ModuleEntryCategory::CoreInit);
	Register<&nox::CoreEntry::GCUpdate>(ModuleEntryCategory::GCUpdate);
	Register<&nox::CoreEntry::Finalize>(ModuleEntryCategory::CoreFinalize);
}

nox::CoreEntry::~CoreEntry()
{
}

void	nox::CoreEntry::Init()
{
	nox::GarbageCollector::CreateInstance();
	nox::test::Test();
}


void	nox::CoreEntry::Finalize()
{
	nox::GarbageCollector::Instance().FrameGC();
	nox::GarbageCollector::DeleteInstance();

}

void	nox::CoreEntry::GCUpdate()
{
	nox::GarbageCollector::Instance().FrameGC();
}