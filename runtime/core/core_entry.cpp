//	Copyright (C) 2024 NOX ENGINE All Rights Rserved.

///	@file	core_entry.cpp
///	@brief	core_entry
#include	"stdafx.h"
#include	"core_entry.h"

#include	"garbage_collector.h"
#include	"resource_manager.h"

#if NOX_DEVELOP
#include	"dev/net/socket_scheduler.h"
#include	"dev/editor_ipc_server.h"
#endif // NOX_DEVELOP

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
#if NOX_DEVELOP
	nox::dev::net::SocketScheduler::CreateInstance();
	nox::dev::net::SocketScheduler::Instance().Initialize();

	nox::dev::editor_ipc::EditorIpcServer& editor_ipc_server = nox::dev::editor_ipc::EditorIpcServer::CreateInstance();
#endif // NOX_DEVELOP
}

void	nox::CoreEntry::Finalize()
{
#if NOX_DEVELOP
	nox::dev::editor_ipc::EditorIpcServer::DeleteInstance();
	nox::dev::net::SocketScheduler::Instance().Finalize();
	nox::dev::net::SocketScheduler::DeleteInstance();
#endif // NOX_DEVELOP


	nox::GarbageCollector::Instance().FrameGC();
	nox::GarbageCollector::DeleteInstance();
}

void	nox::CoreEntry::GCUpdate()
{
	nox::GarbageCollector::Instance().FrameGC();
}