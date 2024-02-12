//	Copyright (C) 2024 NOX ENGINE All Rights Rserved.

///	@file	os_utility.cpp
///	@brief	os_utility
#include	"stdafx.h"
#include	"os_utility.h"

#include	"../if/basic_definition.h"

#if NOX_WINDOWS
#include	"windows.h"
#endif // NOX_WINDOWS


using namespace nox;

os::SystemInfo	os::GetSystemInfo()
{
#if NOX_WINDOWS

#endif // NOX_WINDOWS


	return SystemInfo{
		.hardwareNum = 0
	};
}


uint8 os::GetHardwareConcurrency()
{
#if NOX_WINDOWS

	::SYSTEM_INFO systemInfo;
	::GetSystemInfo(&systemInfo);
	return static_cast<uint8>(systemInfo.dwNumberOfProcessors);
#else
	return -1;
#endif // NOX_WINDOWS
}