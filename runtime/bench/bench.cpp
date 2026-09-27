//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	bench.cpp
///	@brief	計測ハーネスの実装 (実行環境の取得・一括実行・JSON 出力)
///	@details	出力の形式は .github/scripts/bench_common.py の "nox-bench-raw/1"。
///				エンジンのログは std::wcout (標準出力) に出るので、JSON は必ずファイルへ書く。
///				進捗は標準エラーへ出す。

#include	"pch.h"
#include	"bench.h"

#include	<intrin.h>
#include	<realtimeapiset.h>

#include	<array>
#include	<cstdio>
#include	<cstring>
#include	<thread>

namespace
{
	/// @brief DoNotOptimize の受け口。volatile へ書くのでこの関数自体も消えない
	constinit const volatile char* volatile g_sink_pointer = nullptr;
	constinit volatile char g_sink_value = 0;

	/// @brief 実行環境の情報 (JSON の context)
	struct Context final
	{
		std::array<char, 64> cpu{};
		std::array<char, 64> compiler{};
		nox::uint32 logical_cpus = 0u;
		bool hypervisor = false;
		nox::int32 pinned_cpu = -1;
		bool high_priority = false;
		nox::uint64 qpc_hz = 0u;
	};

	/// @brief __cpuid からブランド文字列を取り、前後の空白を落とす
	void ReadCpuBrand(std::array<char, 64>& dest, bool& hypervisor)noexcept
	{
		std::array<int, 4> registers{};
		__cpuid(registers.data(), 1);
		hypervisor = ((static_cast<unsigned int>(registers[2]) >> 31u) & 1u) != 0u;

		std::array<char, 49> brand{};
		__cpuid(registers.data(), static_cast<int>(0x80000000u));
		if (static_cast<unsigned int>(registers[0]) >= 0x80000004u)
		{
			for (unsigned int leaf = 0u; leaf < 3u; ++leaf)
			{
				__cpuid(registers.data(), static_cast<int>(0x80000002u + leaf));
				std::memcpy(brand.data() + (leaf * 16u), registers.data(), 16u);
			}
		}
		brand[48] = '\0';

		const char* begin = brand.data();
		while (*begin == ' ')
		{
			++begin;
		}
		size_t length = std::strlen(begin);
		while (length > 0u && begin[length - 1u] == ' ')
		{
			--length;
		}
		if (length == 0u)
		{
			std::snprintf(dest.data(), dest.size(), "%s", "unknown");
			return;
		}
		std::snprintf(dest.data(), dest.size(), "%.*s", static_cast<int>(length), begin);
	}

	void ReadCompiler(std::array<char, 64>& dest)noexcept
	{
#if defined(__clang__)
		std::snprintf(dest.data(), dest.size(), "Clang %d.%d.%d (clang-cl)", __clang_major__, __clang_minor__, __clang_patchlevel__);
#elif defined(_MSC_FULL_VER)
		std::snprintf(dest.data(), dest.size(), "MSVC %d.%d.%d", _MSC_VER / 100, _MSC_VER % 100, _MSC_FULL_VER % 100000);
#else
		std::snprintf(dest.data(), dest.size(), "%s", "unknown");
#endif
	}

	[[nodiscard]] const char* GetConfigurationName()noexcept
	{
#if NOX_DEBUG
		return "Debug";
#elif NOX_MASTER
		return "Master";
#else
		return "Release";
#endif
	}

	/// @brief 計測スレッドの揺れを減らす
	/// @details プロセスを HIGH 優先度にし、計測スレッドを CPU 1 に固定する。
	///          CPU 0 は割り込み・DPC を受けやすいので避ける。REALTIME は OS のスレッドを
	///          止めてしまうので使わない。仮想マシンではハイパーバイザ側の割り当ては変えられないが、
	///          ゲスト内でのスレッド移動は減らせる。
	void StabilizeThread(Context& context, const bool pin)noexcept
	{
		context.high_priority = (::SetPriorityClass(::GetCurrentProcess(), HIGH_PRIORITY_CLASS) != 0);
		if (pin && context.logical_cpus >= 2u)
		{
			const DWORD_PTR mask = DWORD_PTR{ 1u } << 1u;
			if (::SetThreadAffinityMask(::GetCurrentThread(), mask) != 0u)
			{
				context.pinned_cpu = 1;
			}
		}
	}

	//	---------------------------------------------------------------------------------
	//	JSON の書き出し
	//	---------------------------------------------------------------------------------

	void WriteString(std::FILE* const file, const char* const text)noexcept
	{
		std::fputc('"', file);
		for (const char* cursor = text; *cursor != '\0'; ++cursor)
		{
			const unsigned char c = static_cast<unsigned char>(*cursor);
			switch (c)
			{
			case '"': std::fputs("\\\"", file); break;
			case '\\': std::fputs("\\\\", file); break;
			case '\n': std::fputs("\\n", file); break;
			case '\r': std::fputs("\\r", file); break;
			case '\t': std::fputs("\\t", file); break;
			default:
				if (c < 0x20u)
				{
					std::fprintf(file, "\\u%04x", static_cast<unsigned int>(c));
				}
				else
				{
					//	UTF-8 のバイト列はそのまま書く (ソースは /utf-8 でコンパイルしている)
					std::fputc(static_cast<int>(c), file);
				}
				break;
			}
		}
		std::fputc('"', file);
	}

	void WriteNumber(std::FILE* const file, const double value)noexcept
	{
		if (std::isfinite(value))
		{
			std::fprintf(file, "%.7g", value);
		}
		else
		{
			std::fputs("null", file);
		}
	}

	void WriteNumberArray(std::FILE* const file, const std::span<const double> values)noexcept
	{
		std::fputc('[', file);
		for (size_t index = 0u; index < values.size(); ++index)
		{
			if (index != 0u)
			{
				std::fputc(',', file);
			}
			WriteNumber(file, values[index]);
		}
		std::fputc(']', file);
	}

	/// @brief ベンチ 1 本の結果 (JSON の benchmarks[] の 1 要素)
	struct Record final
	{
		const nox::bench::Definition* definition = nullptr;
		nox::uint32 threads = 1u;
		nox::uint64 ops_per_sample = 0u;
		std::vector<double> samples_ns;
		std::vector<double> cycles_per_op;
		double allocs_per_op = 0.0;
		double alloc_bytes_per_op = 0.0;
		double frees_per_op = 0.0;
		bool alloc_stable = true;
		bool budget_checked = false;
		bool budget_ok = true;
	};

	/// @brief グループ名 (name の先頭の '/' まで) を書く
	void WriteGroup(std::FILE* const file, const char* const name)noexcept
	{
		const char* const slash = std::strchr(name, '/');
		const size_t length = (slash != nullptr) ? static_cast<size_t>(slash - name) : std::strlen(name);
		std::array<char, 64> group{};
		std::snprintf(group.data(), group.size(), "%.*s", static_cast<int>(length), name);
		WriteString(file, group.data());
	}

	[[nodiscard]] bool WriteJson(
		const char* const path,
		const Context& context,
		const nox::bench::Options& options,
		const std::span<const Record> records)noexcept
	{
		std::FILE* file = nullptr;
		if (::fopen_s(&file, path, "wb") != 0 || file == nullptr)
		{
			return false;
		}

		std::fputs("{\"schema\":\"nox-bench-raw/1\",\"harness_version\":1,\"context\":{", file);
		std::fputs("\"cpu\":", file);
		WriteString(file, context.cpu.data());
		std::fprintf(file, ",\"logical_cpus\":%u", context.logical_cpus);
		std::fprintf(file, ",\"hypervisor\":%s", context.hypervisor ? "true" : "false");
		std::fputs(",\"compiler\":", file);
		WriteString(file, context.compiler.data());
		std::fputs(",\"config\":", file);
		WriteString(file, GetConfigurationName());
		std::fprintf(file, ",\"smoke\":%s", options.smoke ? "true" : "false");
		if (context.pinned_cpu >= 0)
		{
			std::fprintf(file, ",\"pinned_cpu\":%d", context.pinned_cpu);
		}
		else
		{
			std::fputs(",\"pinned_cpu\":null", file);
		}
		std::fprintf(file, ",\"priority\":\"%s\"", context.high_priority ? "high" : "normal");
		std::fprintf(file, ",\"qpc_hz\":%llu", static_cast<unsigned long long>(context.qpc_hz));
		std::fputs(",\"alloc_counters\":true},\"benchmarks\":[", file);

		for (size_t index = 0u; index < records.size(); ++index)
		{
			const Record& record = records[index];
			const nox::bench::Definition& definition = *record.definition;
			if (index != 0u)
			{
				std::fputc(',', file);
			}
			std::fputs("\n{\"name\":", file);
			WriteString(file, definition.name);
			std::fputs(",\"group\":", file);
			WriteGroup(file, definition.name);
			std::fputs(",\"title\":", file);
			WriteString(file, definition.title);
			std::fputs(",\"per\":", file);
			WriteString(file, definition.per);
			std::fprintf(file, ",\"threads\":%u", record.threads);
			if (definition.alloc_budget >= 0)
			{
				std::fprintf(file, ",\"alloc_budget\":%d", definition.alloc_budget);
			}
			else
			{
				std::fputs(",\"alloc_budget\":null", file);
			}
			std::fprintf(file, ",\"ops_per_sample\":%llu", static_cast<unsigned long long>(record.ops_per_sample));
			std::fputs(",\"samples_ns\":", file);
			WriteNumberArray(file, record.samples_ns);
			std::fputs(",\"cycles_per_op\":", file);
			if (record.threads == 1u && record.cycles_per_op.empty() == false)
			{
				WriteNumberArray(file, record.cycles_per_op);
			}
			else
			{
				//	複数スレッドで働くベンチでは計測スレッドのサイクルだけ見ても意味が無い
				std::fputs("null", file);
			}
			std::fputs(",\"allocs_per_op\":", file);
			WriteNumber(file, record.allocs_per_op);
			std::fputs(",\"alloc_bytes_per_op\":", file);
			WriteNumber(file, record.alloc_bytes_per_op);
			std::fputs(",\"frees_per_op\":", file);
			WriteNumber(file, record.frees_per_op);
			std::fprintf(file, ",\"alloc_stable\":%s", record.alloc_stable ? "true" : "false");
			if (record.budget_checked)
			{
				std::fprintf(file, ",\"budget_ok\":%s}", record.budget_ok ? "true" : "false");
			}
			else
			{
				std::fputs(",\"budget_ok\":null}", file);
			}
		}
		std::fputs("\n]}\n", file);

		const bool write_ok = (std::ferror(file) == 0);
		const bool close_ok = (std::fclose(file) == 0);
		return write_ok && close_ok;
	}
}

__declspec(noinline) void nox::bench::detail::UseCharPointer(const volatile char* const pointer)noexcept
{
	g_sink_pointer = pointer;
	g_sink_value = *pointer;
}

nox::uint64 nox::bench::detail::ReadThreadCycles()noexcept
{
	ULONG64 cycles = 0u;
	if (::QueryThreadCycleTime(::GetCurrentThread(), &cycles) == 0)
	{
		return 0u;
	}
	return static_cast<nox::uint64>(cycles);
}

int nox::bench::RunBenchmarks(
	const std::span<const nox::bench::Definition* const> definitions,
	const nox::bench::Options& options,
	const char* const output_path)
{
	Context context{};
	ReadCpuBrand(context.cpu, context.hypervisor);
	ReadCompiler(context.compiler);
	context.logical_cpus = static_cast<nox::uint32>(std::thread::hardware_concurrency());
	LARGE_INTEGER frequency{};
	if (::QueryPerformanceFrequency(&frequency) != 0)
	{
		context.qpc_hz = static_cast<nox::uint64>(frequency.QuadPart);
	}
	StabilizeThread(context, true);

	std::fprintf(stderr, "[bench] cpu=%s logical_cpus=%u config=%s compiler=%s pinned_cpu=%d smoke=%d\n",
		context.cpu.data(), context.logical_cpus, GetConfigurationName(), context.compiler.data(),
		context.pinned_cpu, options.smoke ? 1 : 0);

	std::vector<Record> records;
	records.reserve(definitions.size());

	nox::uint32 budget_violation_count = 0u;
	for (const nox::bench::Definition* const definition : definitions)
	{
		std::fprintf(stderr, "[bench] %s ...\n", definition->name);
		std::fflush(stderr);

		nox::bench::State state(options);
		definition->function(state);
		if (state.HasRun() == false)
		{
			//	本体が Run を呼ばずに戻った (前提が満たせず計測を諦めた)。結果には載せない
			std::fprintf(stderr, "[bench] %s: 計測されなかったので飛ばす\n", definition->name);
			continue;
		}

		Record record{};
		record.definition = definition;
		record.threads = state.GetThreads();
		record.ops_per_sample = state.GetOpsPerSample();
		record.samples_ns.assign(state.GetSamplesNs().begin(), state.GetSamplesNs().end());
		record.cycles_per_op.assign(state.GetCyclesPerOp().begin(), state.GetCyclesPerOp().end());
		record.allocs_per_op = state.GetAllocsPerOp();
		record.alloc_bytes_per_op = state.GetAllocBytesPerOp();
		record.frees_per_op = state.GetFreesPerOp();
		record.alloc_stable = state.IsAllocStable();

		//	予算の判定。Debug は STL のデバッグ用確保 (コンテナプロキシ) が混ざるので判定しない。
		//	smoke は op 回数が 1 回で償却が効かないので判定しない。
#if !NOX_DEBUG
		if (options.smoke == false && definition->alloc_budget >= 0)
		{
			record.budget_checked = true;
			record.budget_ok = (record.allocs_per_op <= static_cast<double>(definition->alloc_budget) + 1.0e-9);
			if (record.budget_ok == false)
			{
				++budget_violation_count;
				std::fprintf(stderr, "[bench] %s: 確保回数が予算を超えた (%.3f 回/op, 予算 %d)\n",
					definition->name, record.allocs_per_op, definition->alloc_budget);
			}
		}
#endif // !NOX_DEBUG

		const std::span<const double> samples = record.samples_ns;
		double best = samples.empty() ? 0.0 : samples[0];
		for (const double sample : samples)
		{
			best = (sample < best) ? sample : best;
		}
		std::fprintf(stderr, "[bench] %s: min %.4g ns/%s, allocs %.3g/%s\n",
			definition->name, best, definition->per, record.allocs_per_op, definition->per);

		records.push_back(std::move(record));
	}

	if (WriteJson(output_path, context, options, records) == false)
	{
		std::fprintf(stderr, "[bench] 結果を書き出せなかった: %s\n", output_path);
		return 1;
	}
	std::fprintf(stderr, "[bench] %zu 本の結果を書き出した\n", records.size());

	return (budget_violation_count > 0u) ? 3 : 0;
}
