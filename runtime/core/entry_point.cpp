//	Copyright (C) 2024 NOX ENGINE All Rights Rserved.

///	@file	entry_point.cpp
///	@brief	entry_point
#include	"pch.h"
#include	"entry_point.h"

#include	"application.h"
#include	"log_id.h"

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

	{
		nox::Application application;
		application.Run();
	}

	nox::os::Finalize();

	nox::reflection::Finalize();

	nox::memory::ReleaseBootMemory();
	nox::memory::CheckMemoryLeak();
	nox::memory::Finialize();

	return 0;
}