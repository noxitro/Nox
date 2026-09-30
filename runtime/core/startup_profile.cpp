//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	startup_profile.cpp
///	@brief	起動の区切りの記録と JSON (nox-startup/1) の書き出し
///	@details	JSON を読むのは .github/scripts/bench-run.py。区間の定義 (どの区切りからどこまでを
///				1 本のベンチマークとして見るか) と表示名はそちらに置き、ここは生の値だけを出す。
#include	"pch.h"
#include	"startup_profile.h"

#include	"log_id.h"
#include	"../kernel/memory/allocation_counter.h"

#include	<array>
#include	<cstdio>

namespace
{
	constexpr std::size_t kPointCount = nox::util::ToUnderlying(nox::startup_profile::Point::_Max);

	/// @brief JSON の name。Point の並びと 1 対 1
	constexpr std::array<const char*, kPointCount> kPointNames = {
		"entry_point",
		"memory_initialized",
		"reflection_initialized",
		"os_initialized",
		"world_initialized",
		"init_phase_done",
		"start_phase_done",
		"first_frame_done",
	};
	static_assert(kPointNames.back() != nullptr, "Point を足したら kPointNames にも足す");

	struct Record final
	{
		nox::int64 qpc = 0;
		nox::memory::AllocationCounters counters{};
		bool recorded = false;
	};

	constinit std::array<Record, kPointCount> g_records{};

	/// @brief EntryPoint を記録したときのシステム時刻 (FILETIME、100ns 単位)。プロセスの作成時刻との差を取る
	constinit nox::uint64 g_entry_point_file_time = 0u;

	[[nodiscard]] inline nox::uint64 ToUInt64(const ::FILETIME& file_time)noexcept
	{
		return (static_cast<nox::uint64>(file_time.dwHighDateTime) << 32u) | static_cast<nox::uint64>(file_time.dwLowDateTime);
	}

	/// @brief QPC の刻みをナノ秒にする。刻みに 10^9 を掛けると桁あふれしうるので、秒と端数に分けて掛ける
	[[nodiscard]] inline nox::uint64 TicksToNanoseconds(const nox::uint64 ticks, const nox::uint64 frequency)noexcept
	{
		constexpr nox::uint64 kNanosecondsPerSecond = 1'000'000'000u;
		return ((ticks / frequency) * kNanosecondsPerSecond) + (((ticks % frequency) * kNanosecondsPerSecond) / frequency);
	}

	/// @brief プロセスが作られてから EntryPoint までのナノ秒。取れなければ負
	[[nodiscard]] nox::int64 ResolvePreMainNanoseconds()noexcept
	{
		if (g_entry_point_file_time == 0u)
		{
			return -1;
		}
		::FILETIME creation_file_time{};
		::FILETIME exit_file_time{};
		::FILETIME kernel_file_time{};
		::FILETIME user_file_time{};
		if (::GetProcessTimes(::GetCurrentProcess(), &creation_file_time, &exit_file_time, &kernel_file_time, &user_file_time) == FALSE)
		{
			return -1;
		}
		const nox::uint64 creation_time = ToUInt64(creation_file_time);
		if ((creation_time == 0u) || (creation_time > g_entry_point_file_time))
		{
			return -1;
		}
		return static_cast<nox::int64>((g_entry_point_file_time - creation_time) * 100u);
	}
}

void nox::startup_profile::Mark(const nox::startup_profile::Point point)noexcept
{
	const std::size_t index = nox::util::ToUnderlying(point);
	if (index >= kPointCount)
	{
		NOX_ASSERT(false, u8"範囲外の起動の区切り");
		return;
	}

	Record& record = g_records[index];
	if (record.recorded == true)
	{
		return;
	}

	::LARGE_INTEGER counter{};
	::QueryPerformanceCounter(&counter);
	record.qpc = counter.QuadPart;

	if (point == nox::startup_profile::Point::EntryPoint)
	{
		::FILETIME now{};
		::GetSystemTimePreciseAsFileTime(&now);
		g_entry_point_file_time = ToUInt64(now);
	}

	record.counters = nox::memory::GetAllocationCounters();
	record.recorded = true;
}

void nox::startup_profile::WriteReportIfRequested(const std::span<const nox::char16* const> command_line_args)noexcept
{
	static constexpr std::u16string_view kStartupReportKey = u"--startup-report";

	const std::optional<std::u16string_view> value =
		nox::os::TryGetCommandLineArgValue(command_line_args, kStartupReportKey);
	if ((value.has_value() == false) || (value->empty() == true))
	{
		return;
	}

	const Record& origin = g_records[nox::util::ToUnderlying(nox::startup_profile::Point::EntryPoint)];
	::LARGE_INTEGER frequency{};
	::QueryPerformanceFrequency(&frequency);
	if ((origin.recorded == false) || (frequency.QuadPart <= 0))
	{
		NOX_WARNING_LINE(nox::log_id::CoreCommon, u"起動の記録が無いので --startup-report を書かない");
		return;
	}

	//	値は引数文字列の末尾をそのまま指しているので、ヌル終端している
	std::FILE* file = nullptr;
	if ((::_wfopen_s(&file, reinterpret_cast<const wchar_t*>(value->data()), L"wb") != 0) || (file == nullptr))
	{
		NOX_WARNING_LINE(nox::log_id::CoreCommon, u"--startup-report のファイルを開けない");
		return;
	}

	const nox::uint64 qpc_hz = static_cast<nox::uint64>(frequency.QuadPart);
	std::fprintf(file, "{\"schema\":\"nox-startup/1\",\"qpc_hz\":%llu,\"pre_main_ns\":",
		static_cast<unsigned long long>(qpc_hz));
	const nox::int64 pre_main_ns = ResolvePreMainNanoseconds();
	if (pre_main_ns >= 0)
	{
		std::fprintf(file, "%lld", static_cast<long long>(pre_main_ns));
	}
	else
	{
		std::fputs("null", file);
	}

	std::fputs(",\"points\":[", file);
	for (std::size_t index = 0u; index < kPointCount; ++index)
	{
		const Record& record = g_records[index];
		std::fprintf(file, "%s{\"name\":\"%s\"", (index == 0u) ? "" : ",", kPointNames[index]);
		if ((record.recorded == true) && (record.qpc >= origin.qpc))
		{
			const nox::uint64 elapsed_ticks = static_cast<nox::uint64>(record.qpc - origin.qpc);
			std::fprintf(file, ",\"ns\":%llu,\"allocs\":%llu,\"alloc_bytes\":%llu,\"frees\":%llu}",
				static_cast<unsigned long long>(TicksToNanoseconds(elapsed_ticks, qpc_hz)),
				static_cast<unsigned long long>(record.counters.allocate_count),
				static_cast<unsigned long long>(record.counters.allocate_bytes),
				static_cast<unsigned long long>(record.counters.deallocate_count));
		}
		else
		{
			//	そこまで到達しなかった (途中で終了した) 区切り
			std::fputs(",\"ns\":null}", file);
		}
	}
	std::fputs("]}\n", file);
	std::fclose(file);
}
