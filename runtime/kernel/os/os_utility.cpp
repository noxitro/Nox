//	Copyright (C) 2024 NOX ENGINE All Rights Rserved.

///	@file	os_utility.cpp
///	@brief	os_utility
#include	"stdafx.h"
#include	"os_utility.h"

#include	"../basic_definition.h"

#include	"assertion.h"

#include	"string_format.h"

#if NOX_WINDOWS
#include	"windows.h"
#endif // NOX_WINDOWS


nox::os::SystemInfo	nox::os::GetSystemInfo()
{
#if NOX_WINDOWS

#endif // NOX_WINDOWS
	
	return SystemInfo{
		.hardwareNum = 0
	};
}


nox::uint8 nox::os::GetHardwareConcurrency()
{
#if NOX_WINDOWS

	::SYSTEM_INFO systemInfo;
	::GetSystemInfo(&systemInfo);
	return static_cast<uint8>(systemInfo.dwNumberOfProcessors);
#else
	return -1;
#endif // NOX_WINDOWS
}

void* nox::os::detail::GetProcAddressImpl(void* const moduleHandle, const char* const procNamePtr)
{
	::FARPROC const proc = ::GetProcAddress(
		static_cast<::HMODULE>(moduleHandle), procNamePtr);

	NOX_ASSERT(proc != nullptr, util::Format(U"dll読み込みに失敗 procName = {0}", procNamePtr));

	return proc;
}


void* nox::os::LoadDLL(std::u16string_view path, void* handlePtr, const uint32 flags)
{
	const ::HMODULE moduleHandlePtr = ::LoadLibraryExW(util::CharCast<wchar16>(path.data()), handlePtr, static_cast<::DWORD>(flags));

	NOX_ASSERT(moduleHandlePtr != nullptr, util::Format(U"dll読み込みに失敗 path = {0}", path));

	return moduleHandlePtr;
}

bool nox::os::UnloadDLL(nox::not_null<void*> moduleHandlePtr)
{
	return ::FreeLibrary(static_cast<::HMODULE>(moduleHandlePtr.get()));
}