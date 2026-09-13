// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

///	@file	os.cpp
///	@brief	os
#include	"pch.h"

#include	"os.h"
#include	"atomic.h"
#include	"thread.h"
#include	"assertion.h"
#include	"unicode_converter.h"
#include	"preprocessor/util.h"

#if NOX_WINDOWS
#include	<Psapi.h>
#include	"os/window.h"
#endif // NOX_WINDOWS

#include	"../parallel_execute_checker.h"

namespace nox::os
{
	namespace
	{
		/// @brief 起動時引数リスト
		constinit std::span<const nox::char16* const> command_line_args_;

		/// @brief os関数を初期化したネイティブスレッドID
		constinit nox::FunctionResultType<decltype(&nox::os::Thread::GetNativeThreadId)> native_thread_id_ = {};

		/// @brief 
		constinit void(*window_dispatch_function_)(const void*) = nullptr;
		constinit const void* window_dispatch_arg_ = nullptr;

#if !NOX_MASTER
		constinit nox::util::ParallelExecuteChecker parallel_execute_checker_ = {};
#endif // !NOX_MASTER
	}
}

void	nox::os::Initialize(const std::span<const nox::char16* const> args)
{
	nox::os::command_line_args_ = args;
	
	native_thread_id_ = nox::os::Thread::GetThisThreadNativeThreadId();
}

bool	nox::os::Update()
{
	::MSG msg;

	// キューにあるものだけ処理。空なら抜ける
	while (::PeekMessageW(&msg, nullptr, 0U, 0U, PM_REMOVE))
	{
		if (msg.message == WM_QUIT)
		{
			return false; // アプリ終了
		}
		::TranslateMessage(&msg);
		::DispatchMessageW(&msg);
	}

	// ディスパッチ要求があれば処理
	if (window_dispatch_function_ != nullptr)
	{
		window_dispatch_function_(window_dispatch_arg_);
		window_dispatch_function_ = nullptr;
		window_dispatch_arg_ = nullptr;
	}

	return true;
}

void	nox::os::Finalize()
{
	nox::os::command_line_args_ = {};
}

std::span<const nox::char16* const> nox::os::GetCommandLineArgList() noexcept
{
	return nox::os::command_line_args_;
}

namespace
{
	//	キーと値の区切りに使える文字。
	[[nodiscard]] inline constexpr bool IsCommandLineArgSeparator(const nox::char16 character)noexcept
	{
		return (character == u'=') || (character == u':');
	}

	///	@brief		引数がキーに一致するなら、キーの直後(区切り文字を含む)を返す。
	///	@details	「キーで始まる」だけの判定では --foo が --foobar にも一致してしまうので、
	///				キーの直後が区切り文字か終端であることまで確かめる。
	[[nodiscard]] inline std::optional<std::u16string_view> TryMatchCommandLineArgKey(
		const std::u16string_view command_line_arg,
		const std::u16string_view key)noexcept
	{
		if (command_line_arg.starts_with(key) == false)
		{
			return std::nullopt;
		}

		const std::u16string_view remainder = command_line_arg.substr(key.size());
		if ((remainder.empty() == true) || (IsCommandLineArgSeparator(remainder.front()) == true))
		{
			return remainder;
		}
		return std::nullopt;
	}
}

std::optional<std::u16string_view> nox::os::TryGetCommandLineArgValue(
	const std::span<const nox::char16* const> command_line_args,
	const std::u16string_view key)noexcept
{
	for (const nox::char16* const command_line_arg : command_line_args)
	{
		if (command_line_arg == nullptr)
		{
			continue;
		}

		const std::optional<std::u16string_view> remainder =
			TryMatchCommandLineArgKey(std::u16string_view(command_line_arg), key);
		if (remainder.has_value() == false)
		{
			continue;
		}

		//	区切り文字は値に含めない。--foo=bar なら "bar" を返す。
		//	--foo だけなら値は空文字列 (キーは在ったので nullopt にはしない)。
		return (remainder->empty() == true) ? *remainder : remainder->substr(1u);
	}
	return std::nullopt;
}

bool nox::os::ContainsCommandLineArgKey(
	const std::span<const nox::char16* const> command_line_args,
	const std::u16string_view arg)noexcept
{
	return nox::os::TryGetCommandLineArgValue(command_line_args, arg).has_value();
}

bool nox::os::ContainsCommandLineArgKey(std::u16string_view arg)noexcept
{
	return nox::os::ContainsCommandLineArgKey(nox::os::command_line_args_, arg);
}

std::optional<std::u16string_view> nox::os::GetCommandLineArgValue(std::u16string_view key)noexcept
{
	return nox::os::TryGetCommandLineArgValue(nox::os::command_line_args_, key);
}

nox::StlU16String	nox::os::GetDirectoryUTF8()
{
	std::array<nox::wchar16, nox::os::k_max_path_length> buffer;
	NOX_ASSERT(::GetCurrentDirectoryW(nox::os::k_max_path_length, buffer.data()) != NULL, u"GetCurrentDirectoryW failed");

	return nox::StlU16String(reinterpret_cast<const char16*>(buffer.data()));
}

nox::U16String	nox::os::GetDirectory()
{
	std::array<nox::char16, nox::os::k_max_path_length> buffer;
	return nox::U16String(GetDirectory(buffer));
}

std::u16string_view	nox::os::GetDirectory(std::span<nox::char16> dest_buffer)
{
	std::array<nox::wchar16, nox::os::k_max_path_length> native_buffer;
	NOX_ASSERT(::GetCurrentDirectoryW(nox::os::k_max_path_length, native_buffer.data()) != NULL, u"GetCurrentDirectoryW failed");

	nox::unicode::ConvertU16String(native_buffer.data(), dest_buffer);
	return std::u16string_view(dest_buffer.data(), dest_buffer.size());
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

	NOX_ASSERT(isSuccess == TRUE, u"GetProcessMemoryInfoに失敗しました");

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

void	nox::os::Sleep(const uint32 milliseconds)
{
#if NOX_WINDOWS
	::Sleep(milliseconds);
#else
	static_assert(false);
#endif // NOX_WINDOWS
}

void	nox::os::detail::DispatchCreateNativeWindow(void(*func)(const void*), const void* arg)
{
#if !NOX_MASTER
	NOX_LOCAL_SCOPE(nox::util::ParallelExecuteCheckScope(nox::os::parallel_execute_checker_));
#endif

	while (nox::os::window_dispatch_function_ != nullptr)
	{
		nox::os::Thread::Sleep(1);
	}

	nox::os::window_dispatch_arg_ = arg;
	nox::os::window_dispatch_function_ = func;

	//	実行されるまで待つ？
	while (nox::os::window_dispatch_function_ != nullptr)
	{
		nox::os::Thread::Sleep(1);
	}
}