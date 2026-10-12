//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	core_module.cpp
///	@brief	core_module
#include	"pch.h"
#include	"core_module.h"

#include	"world_registery.h"
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

void CreateEntityLogics()
{

}

void nox::CoreModule::Register(nox::WorldRegistery& registry)const
{
	registry.Register<
		nox::SceneManager,
		nox::GarbageCollector,
#if NOX_DEVELOP
		nox::dev::net::SocketScheduler,
		nox::dev::editor_remote::EditorRemoteServer
#endif
	>();
}