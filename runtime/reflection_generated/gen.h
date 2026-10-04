//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	gen.h
///	@brief	gen
#pragma once

namespace nox::reflection::gen
{
#if NOX_DEBUG
#if NOX_WIN64
	void	Register_x64_Debug();
	void	Unregister_x64_Debug();
#endif // NOX_WIN64
#endif // NOX_DEBUG


#if NOX_RELEASE
#if NOX_WIN64
	void	Register_x64_Release();
	void	Unregister_x64_Release();
#endif // NOX_WIN64
#endif // NOX_RELEASE

#if NOX_MASTER
#if NOX_WIN64
	void	Register_x64_Master();
	void	Unregister_x64_Master();
#endif // NOX_WIN64
#endif // NOX_MASTER
}