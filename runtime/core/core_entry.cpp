//	Copyright (C) 2024 NOX ENGINE All Rights Rserved.

///	@file	core_entry.cpp
///	@brief	core_entry
#include	"pch.h"
#include	"core_entry.h"

#include	"garbage_collector.h"
#include	"resource_manager.h"
#include	"scene_manager.h"

#if NOX_DEVELOP
#include	"dev/net/socket_scheduler.h"
#include	"dev/editor_remote_server.h"
#endif // NOX_DEVELOP

nox::CoreEntry::CoreEntry()
{
	Register<&nox::CoreEntry::Init>(ModuleEntryCategory::CoreInit);
	Register<&nox::CoreEntry::GCUpdate>(ModuleEntryCategory::GCUpdate);
	Register<&nox::CoreEntry::Finalize>(ModuleEntryCategory::CoreFinalize);
	Register<&nox::CoreEntry::SocketUpdate>(ModuleEntryCategory::SocketUpdate);
}

nox::CoreEntry::~CoreEntry()
{
}

void	nox::CoreEntry::Init()
{
	nox::GarbageCollector::CreateInstance();
	auto& scene_manager = nox::SceneManager::CreateInstance();
	scene_manager.Initialize(u8"main_scene.scn.json");
#if NOX_DEVELOP
	nox::dev::net::SocketScheduler::CreateInstance();
	nox::dev::net::SocketScheduler::Instance().Initialize();

	nox::dev::editor_remote::EditorRemoteServer& editor_remote_server = nox::dev::editor_remote::EditorRemoteServer::CreateInstance();
#endif // NOX_DEVELOP
}

void	nox::CoreEntry::Finalize()
{
#if NOX_DEVELOP
	nox::dev::editor_remote::EditorRemoteServer::DeleteInstance();
	nox::dev::net::SocketScheduler::Instance().Finalize();
	nox::dev::net::SocketScheduler::DeleteInstance();
#endif // NOX_DEVELOP

	nox::SceneManager::Instance().Finalize();
	nox::SceneManager::DeleteInstance();
	nox::GarbageCollector::Instance().FrameGC();
	nox::GarbageCollector::DeleteInstance();
}

void	nox::CoreEntry::SocketUpdate()
{
#if NOX_DEVELOP
	nox::dev::editor_remote::EditorRemoteServer::Instance().Update();
#endif // NOX_DEVELOP

}

void	nox::CoreEntry::GCUpdate()
{
	nox::GarbageCollector::Instance().FrameGC();
}