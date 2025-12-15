//	Copyright (C) 2024 NOX ENGINE All Rights Rserved.

///	@file	entry_point.cpp
///	@brief	entry_point
#include	"stdafx.h"
#include	"entry_point.h"

#include	"application.h"

nox::int32 nox::EntryPoint(const std::span<const char16* const> args)
{
	//	メモリリークチェック
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

	nox::memory::Initialize(true);

	nox::reflection::Initialize();

	nox::os::Initialize(args);

	nox::Application::CreateInstance();

	nox::Application::Instance().Run();

	nox::Application::DeleteInstance();

	nox::os::Finalize();

	nox::reflection::Finalize();

	nox::memory::ReleaseBootMemory();
	nox::memory::CheckMemoryLeak();
	nox::memory::Finialize();

	return 0;
}