//	Copyright (C) 2024 NOX ENGINE All Rights Rserved.

///	@file	entry_point.cpp
///	@brief	entry_point
#include	"stdafx.h"
#include	"entry_point.h"

#include	"application.h"
using namespace nox;

int32 nox::EntryPoint(const std::span<const char16* const> args)
{
	
	//	メモリリークチェック
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	//_CrtSetBreakAlloc(160);
	
	reflection::Initialize();

	os::Initialize(args);

	Application::CreateInstance();

	Application::Instance().Run();

	Application::DeleteInstance();

	os::Finalize();

	reflection::Finalize();

	return 0;
}