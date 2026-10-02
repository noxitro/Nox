//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	entry_point.cpp
///	@brief	entry_point
#include	"pch.h"
#include	"entry_point.h"

//#include	"application.h"
#include	"world.h"
#include	"log_id.h"
#include	"log_service.h"
#include	"startup_profile.h"

nox::int32 nox::EntryPoint(const std::span<const nox::char16* const> args)
{
	//	ここより前 (OS のローダ・DLL・静的初期化) はプロセスの作成時刻との差で測る
	nox::startup_profile::Mark(nox::startup_profile::Point::EntryPoint);

	//	runtime開始を通知
	NOX_INFO_LINE(nox::log_id::CoreCommon, u"================================");
	NOX_INFO_LINE(nox::log_id::CoreCommon, u"=== NOX ENGINE RUNTIME START ===");
	NOX_INFO_LINE(nox::log_id::CoreCommon, u"================================\n\n");

	std::setlocale(LC_CTYPE, "");

	//	メモリリークチェック
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

	nox::memory::Initialize(std::numeric_limits<nox::int32>::max(), true);
	nox::startup_profile::Mark(nox::startup_profile::Point::MemoryInitialized);

	nox::reflection::Initialize();
	nox::startup_profile::Mark(nox::startup_profile::Point::ReflectionInitialized);

	nox::os::Initialize(args);
	nox::startup_profile::Mark(nox::startup_profile::Point::OsInitialized);

	{
		nox::World world;
		world.Run();
	}

	//	CI の起動計測 (--startup-report=<パス>) 用。World の後始末まで済んでから書く
	nox::startup_profile::WriteReportIfRequested(args);

	nox::os::Finalize();

	nox::reflection::Finalize();

	nox::memory::ReleaseBootMemory();
	nox::memory::CheckMemoryLeak();
	nox::memory::Finialize();

	nox::debug::DetachLogHandler();
	return 0;
}
