//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	register.cpp
///	@brief	register
#include	"pch.h"
#include	"register.h"

#include	"gen.h"

#if NOX_DEBUG
#if NOX_WIN64

#endif
#endif

void	nox::reflection::InitializeGen()
{
#if NOX_DEBUG
#if NOX_WIN64
	nox::reflection::gen::Register_x64_Debug();
#endif // NOX_WIN64
#endif // NOX_DEBUG

#if NOX_RELEASE
#if NOX_WIN64
	nox::reflection::gen::Register_x64_Release();
#endif // NOX_WIN64
#endif // NOX_RELEASE

	//	Master の分岐が抜けていて、Master ではクラスが 1 つも登録されなかった。
	//	World::Init が根 (ReflectionObject) のクラスを探すときに見つからず、Master では
	//	アサートが消えるので end() を参照して落ちていた (CI の runtime.exe 起動・終了で発覚)。
#if NOX_MASTER
#if NOX_WIN64
	nox::reflection::gen::Register_x64_Master();
#endif // NOX_WIN64
#endif // NOX_MASTER
}

void	nox::reflection::FinalizeGen()
{
#if NOX_DEBUG
#if NOX_WIN64
	nox::reflection::gen::Unregister_x64_Debug();
#endif // NOX_WIN64
#endif // NOX_DEBUG

#if NOX_RELEASE
#if NOX_WIN64
	nox::reflection::gen::Unregister_x64_Release();
#endif // NOX_WIN64
#endif // NOX_RELEASE

#if NOX_MASTER
#if NOX_WIN64
	nox::reflection::gen::Unregister_x64_Master();
#endif // NOX_WIN64
#endif // NOX_MASTER
}