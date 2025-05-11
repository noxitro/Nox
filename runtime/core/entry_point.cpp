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
//	_CrtSetBreakAlloc(182);

	nox::memory::Initialize(true);

	reflection::Initialize();

	os::Initialize(args);

	Application::CreateInstance();

	int* p = new int();
	Application::Instance().Run();

	Application::DeleteInstance();

	os::Finalize();

	reflection::Finalize();

	nox::memory::ReleaseBootMemory();
	nox::memory::CheckMemoryLeak();
	nox::memory::Finialize();

	return 0;
}