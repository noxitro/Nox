///	@file	os.cpp
///	@brief	os
#include	"stdafx.h"

#include	"os.h"
#include	"atomic.h"
#include	"thread.h"

#if NOX_WINDOWS
#include	"detail/win64.h"
#include	<Psapi.h>
#endif // NOX_WINDOWS




using namespace nox;

namespace
{
	/// @brief 引数
	std::span<const char16* const> command_line_args_;
}

void	os::Initialize(const std::span<const char16* const> args)
{
	command_line_args_ = args;
}

void	os::Finalize()
{
	command_line_args_ = {};
}

std::span<const char16* const> nox::os::GetCommandLineArgList() noexcept
{
	return command_line_args_;
}

U16String	os::GetDirectoryUTF8()
{
	std::array<wchar16, MAX_PATH_LENGTH> buffer;
	NOX_ASSERT(::GetCurrentDirectoryW(MAX_PATH_LENGTH, buffer.data()) != NULL, U"GetCurrentDirectoryW failed");

	return U16String(reinterpret_cast<const char16*>(buffer.data()));
}

os::ProcessMemoryInfo os::GetCurrentProcessMemoryInfo()
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