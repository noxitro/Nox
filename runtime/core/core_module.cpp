//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	core_module.cpp
///	@brief	core_module
#include	"pch.h"
#include	"core_module.h"

#include	"garbage_collector.h"
#include	"asset_manager.h"
#include	"scene_manager.h"

#if NOX_DEVELOP
#include	"dev/net/socket_scheduler.h"
#include	"dev/editor_remote_server.h"
#endif // NOX_DEVELOP

nox::CoreModule::CoreModule()
{
}

nox::CoreModule::~CoreModule()
{
}

void nox::CoreModule::CreateEngineSystems(nox::PmrVector<nox::SystemBase*>& out)const
{
	out.emplace_back(new nox::SceneManager());
	out.emplace_back(new nox::AssetManager());
	out.emplace_back(new nox::GarbageCollector());
#if NOX_DEVELOP
	out.emplace_back(new nox::dev::net::SocketScheduler());
	out.emplace_back(new nox::dev::editor_remote::EditorRemoteServer());
#endif // NOX_DEVELOP
}