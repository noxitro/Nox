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

}

void	nox::reflection::FinalizeGen()
{
#if NOX_DEBUG
#if NOX_WIN64
	nox::reflection::gen::Unregister_x64_Debug();
#endif // NOX_WIN64
#endif // NOX_DEBUG
}