//	Copyright (C) 2024 NOX ENGINE All Rights Rserved.

///	@file	entry_point.cpp
///	@brief	entry_point
#include	"pch.h"
#include	"entry_point.h"

//#include	"application.h"
#include	"world.h"
#include	"log_id.h"
#include	"log_service.h"

#if !NOX_MASTER
#include	"test/test.h"
#endif // !NOX_MASTER

nox::int32 nox::EntryPoint(const std::span<const nox::char16* const> args)
{
	//	runtime開始を通知
	NOX_INFO_LINE(nox::log_id::CoreCommon, u"================================");
	NOX_INFO_LINE(nox::log_id::CoreCommon, u"=== NOX ENGINE RUNTIME START ===");
	NOX_INFO_LINE(nox::log_id::CoreCommon, u"================================\n\n");

	std::setlocale(LC_CTYPE, "");

	//	メモリリークチェック
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

	nox::memory::Initialize(std::numeric_limits<nox::int32>::max(), true);

	nox::reflection::Initialize();

	nox::os::Initialize(args);

#if !NOX_MASTER
	//	ECS基盤のセルフテスト。Archetypeストレージと引数リストの束縛が壊れていれば起動時点で落ちる。
	nox::test::TestEntityEcs();
	//	ジョブシステムのセルフテスト。配分・完了待ち・0ワーカーのフォールバックを起動時に確認する。
	nox::test::TestJobSystem();
#endif // !NOX_MASTER

	{
		nox::World world;
		world.Run();
	}

	nox::os::Finalize();

	nox::reflection::Finalize();

	nox::memory::ReleaseBootMemory();
	nox::memory::CheckMemoryLeak();
	nox::memory::Finialize();

	nox::debug::DetachLogHandler();
	return 0;
}
