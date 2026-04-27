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
}

nox::CoreEntry::~CoreEntry()
{
}

void nox::CoreEntry::CreateEngineSystems(nox::PmrVector<nox::EngineSystem*>& out)const
{
	out.emplace_back(new nox::SceneManager());
	out.emplace_back(new nox::ResourceManager());
	out.emplace_back(new nox::GarbageCollector());
#if NOX_DEVELOP
	out.emplace_back(new nox::dev::net::SocketScheduler());
	out.emplace_back(new nox::dev::editor_remote::EditorRemoteServerSystem());
#endif // NOX_DEVELOP
}