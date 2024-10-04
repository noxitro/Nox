///	@file	os.cpp
///	@brief	os
#include	"stdafx.h"

#include	"os.h"
#include	"atomic.h"
#include	"thread.h"

#if NOX_WINDOWS
#include	"os/window.h"
#include	<Psapi.h>
#endif // NOX_WINDOWS


namespace
{
	/// @brief 引数
	std::span<const nox::char16* const> command_line_args_;
}

void	nox::os::Initialize(const std::span<const nox::char16* const> args)
{
	command_line_args_ = args;
}

void	nox::os::Finalize()
{
	command_line_args_ = {};
}

std::span<const nox::char16* const> nox::os::GetCommandLineArgList() noexcept
{
	return command_line_args_;
}

nox::U16String	nox::os::GetDirectoryUTF8()
{
	std::array<nox::wchar16, nox::os::MAX_PATH_LENGTH> buffer;
	NOX_ASSERT(::GetCurrentDirectoryW(nox::os::MAX_PATH_LENGTH, buffer.data()) != NULL, U"GetCurrentDirectoryW failed");

	return nox::U16String(reinterpret_cast<const char16*>(buffer.data()));
}

nox::String	nox::os::GetDirectory()
{
	std::array<nox::char32, nox::os::MAX_PATH_LENGTH> buffer;
	return nox::String(GetDirectory(buffer));
}

nox::StringView	nox::os::GetDirectory(std::span<nox::char32> dest_buffer)
{
	std::array<nox::wchar16, nox::os::MAX_PATH_LENGTH> native_buffer;
	NOX_ASSERT(::GetCurrentDirectoryW(nox::os::MAX_PATH_LENGTH, native_buffer.data()) != NULL, U"GetCurrentDirectoryW failed");

	nox::unicode::ConvertU32String(native_buffer.data(), dest_buffer);
	return nox::StringView(dest_buffer.data(), dest_buffer.size());
}

nox::os::ProcessMemoryInfo nox::os::GetCurrentProcessMemoryInfo()
{
	::HANDLE hProc = ::GetCurrentProcess();
	::PROCESS_MEMORY_COUNTERS_EX2 pmc;

	const ::BOOL isSuccess = ::GetProcessMemoryInfo(
		hProc,
		reinterpret_cast<::PROCESS_MEMORY_COUNTERS*>(&pmc),
		sizeof(pmc));

	::CloseHandle(hProc);

	NOX_ASSERT(isSuccess == TRUE, U"GetProcessMemoryInfoに失敗しました");

	return ProcessMemoryInfo
	{
		.cb = pmc.cb,
		.page_fault_count = pmc.PageFaultCount,
		.peak_working_set_size = pmc.PeakWorkingSetSize,
		.working_set_size = pmc.WorkingSetSize,
		.quota_peak_paged_pool_usage = pmc.QuotaPeakPagedPoolUsage,
		.quota_paged_pool_usage = pmc.QuotaPagedPoolUsage,
		.quota_peak_non_paged_pool_usage = pmc.QuotaPeakNonPagedPoolUsage,
		.quota_non_paged_pool_usage = pmc.QuotaNonPagedPoolUsage,
		.pagefile_usage = pmc.PagefileUsage,
		.peak_pagefile_usage = pmc.PeakPagefileUsage,
		.private_usage = pmc.PrivateUsage,
		.private_working_set_size = pmc.PrivateWorkingSetSize,
		.shared_commit_usage = pmc.SharedCommitUsage
	};
}