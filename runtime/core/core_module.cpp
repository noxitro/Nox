//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	core_module.cpp
///	@brief	core_module
#include	"pch.h"
#include	"core_module.h"

#include	"garbage_collector.h"
#include	"asset_manager.h"
#include	"scene_manager.h"
#include	"world.h"

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

void nox::CoreModule::RegisterServices(nox::World& world)const
{
	//	所有権は World に移る。初期化順は登録順ではなく Depends で決まる
	//	(開発ビルドでは SocketScheduler → EditorRemoteServer → AssetManager の順に初期化され、逆順に終了する)。
	world.RegisterService(*new nox::SceneManager());
	world.RegisterService(*new nox::AssetManager());
#if NOX_DEVELOP
	world.RegisterService(*new nox::dev::net::SocketScheduler());
	world.RegisterService(*new nox::dev::editor_remote::EditorRemoteServer());
#endif // NOX_DEVELOP
}

void nox::CoreModule::CreateEngineSystems(nox::PmrVector<nox::SystemBase*>& out)const
{
	out.emplace_back(new nox::GarbageCollector());
}