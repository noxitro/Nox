//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	os_utility.cpp
///	@brief	os_utility
#include	"pch.h"
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

nox::uint32 nox::os::GetLogicalProcessorCount()noexcept
{
#if NOX_WINDOWS
	//	プロセッサグループを跨ぐ環境でも正しい数を返す。
	const ::DWORD count = ::GetActiveProcessorCount(ALL_PROCESSOR_GROUPS);
	if (count != 0u)
	{
		return static_cast<nox::uint32>(count);
	}

	::SYSTEM_INFO system_info;
	::GetSystemInfo(&system_info);
	return system_info.dwNumberOfProcessors != 0u
		? static_cast<nox::uint32>(system_info.dwNumberOfProcessors)
		: 1u;
#else
	return 1u;
#endif // NOX_WINDOWS
}

void* nox::os::detail::GetProcAddressImpl(void* const moduleHandle, const char* const procNamePtr)
{
	::FARPROC const proc = ::GetProcAddress(
		static_cast<::HMODULE>(moduleHandle), procNamePtr);

	NOX_ASSERT(proc != nullptr, u"dll読み込みに失敗 procName = {0}", procNamePtr);

	return proc;
}


void* nox::os::LoadDLL(std::u16string_view path, void* handlePtr, const uint32 flags)
{
	const ::HMODULE moduleHandlePtr = ::LoadLibraryExW(util::CharCast<wchar16>(path.data()), handlePtr, static_cast<::DWORD>(flags));

	NOX_ASSERT(moduleHandlePtr != nullptr, util::Format(u"dll読み込みに失敗 path = {0}", path));

	return moduleHandlePtr;
}

bool nox::os::UnloadDLL(nox::not_null<void*> moduleHandlePtr)
{
	return ::FreeLibrary(static_cast<::HMODULE>(moduleHandlePtr.get()));
}