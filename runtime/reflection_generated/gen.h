//	Copyright (C) 2024 NOX ENGINE All Rights Rserved.

///	@file	gen.h
///	@brief	gen
#pragma once

namespace nox::reflection::gen
{
#if NOX_DEBUG
#if NOX_WIN64
	void	RegisterDebugX64();
	void	UnregisterDebugX64();
#endif // NOX_WIN64
#endif // NOX_DEBUG


#if NOX_RELEASE
#if NOX_WIN64
	void	RegisterReleaseX64();
	void	UnregisterReleaseX64();
#endif // NOX_WIN64
#endif // NOX_RELEASE

#if NOX_MASTER
#if NOX_WIN64
	void	RegisterMasterX64();
	void	UnregisterMasterX64();
#endif // NOX_WIN64
#endif // NOX_MASTER
}