//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	main.cpp
///	@brief	bench_test.exe の入口
///	@details	使い方:
///				  bench_test.exe --out <path> [--filter <部分文字列>] [--smoke] [--samples N]
///				                 [--min-sample-ms X] [--no-pin] [--list]
///				終了コード: 0 成功 / 1 出力失敗 / 2 引数の誤り / 3 確保回数の予算超過
///
///				gtest を使わないので test_support (operator new の CRT 差し替え) を取り込まない。
///				グローバル operator new は runtime.exe と同じく kernel の nox::memory へ流れる。

#include	"pch.h"
#include	"bench.h"

#include	<algorithm>
#include	<array>
#include	<cstdio>
#include	<cstdlib>
#include	<cstring>
#include	<limits>
#include	<string_view>

namespace
{
	/// @brief 登録できるベンチの上限 (一覧はヒープを使わず固定長で持つ)
	constexpr size_t kMaxBenchmarkCount = 128u;

	void PrintUsage()noexcept
	{
		std::fputs(
			"usage: bench_test.exe --out <path> [--filter <substring>] [--smoke] [--samples N]\n"
			"                      [--min-sample-ms X] [--no-pin] [--list]\n",
			stderr);
	}

	/// @brief 正の整数を読む。失敗したら false
	[[nodiscard]] bool ParseUnsigned(const char* const text, nox::uint32& out)noexcept
	{
		char* end = nullptr;
		const unsigned long value = std::strtoul(text, &end, 10);
		if (end == text || *end != '\0' || value == 0u || value > 100000u)
		{
			return false;
		}
		out = static_cast<nox::uint32>(value);
		return true;
	}

	/// @brief 正の小数を読む。失敗したら false
	[[nodiscard]] bool ParsePositive(const char* const text, double& out)noexcept
	{
		char* end = nullptr;
		const double value = std::strtod(text, &end);
		if (end == text || *end != '\0' || !(value > 0.0) || value > 10000.0)
		{
			return false;
		}
		out = value;
		return true;
	}
}

int main(int argc, char** argv)
{
	nox::bench::Options options{};
	const char* output_path = nullptr;
	const char* filter = nullptr;
	bool list_only = false;

	for (int index = 1; index < argc; ++index)
	{
		const std::string_view argument(argv[index]);
		const bool has_value = (index + 1 < argc);
		if (argument == "--out" && has_value)
		{
			output_path = argv[++index];
		}
		else if (argument == "--filter" && has_value)
		{
			filter = argv[++index];
		}
		else if (argument == "--samples" && has_value)
		{
			if (ParseUnsigned(argv[++index], options.sample_count) == false)
			{
				PrintUsage();
				return 2;
			}
		}
		else if (argument == "--min-sample-ms" && has_value)
		{
			double milliseconds = 0.0;
			if (ParsePositive(argv[++index], milliseconds) == false)
			{
				PrintUsage();
				return 2;
			}
			options.min_sample_ns = milliseconds * 1.0e6;
		}
		else if (argument == "--smoke")
		{
			options.smoke = true;
		}
		else if (argument == "--no-pin")
		{
			options.pin = false;
		}
		else if (argument == "--list")
		{
			list_only = true;
		}
		else
		{
			std::fprintf(stderr, "不明な引数: %s\n", argv[index]);
			PrintUsage();
			return 2;
		}
	}

	//	一覧を集めて名前順に並べる (出力の並びを登録順やリンク順に左右させない)
	std::array<const nox::bench::Definition*, kMaxBenchmarkCount> definitions{};
	size_t definition_count = 0u;
	const std::array<std::span<const nox::bench::Definition>, 4> groups{
		nox::bench::GetEcsBenchmarks(),
		nox::bench::GetJobBenchmarks(),
		nox::bench::GetMemoryBenchmarks(),
		nox::bench::GetKernelBenchmarks(),
	};
	for (const std::span<const nox::bench::Definition> group : groups)
	{
		for (const nox::bench::Definition& definition : group)
		{
			if (filter != nullptr && std::strstr(definition.name, filter) == nullptr)
			{
				continue;
			}
			if (definition_count >= definitions.size())
			{
				std::fputs("ベンチの数が上限を超えた (kMaxBenchmarkCount を増やすこと)\n", stderr);
				return 2;
			}
			definitions[definition_count++] = &definition;
		}
	}
	std::sort(definitions.begin(), definitions.begin() + static_cast<std::ptrdiff_t>(definition_count),
		[](const nox::bench::Definition* const left, const nox::bench::Definition* const right)noexcept
		{
			return std::strcmp(left->name, right->name) < 0;
		});

	if (list_only)
	{
		for (size_t index = 0u; index < definition_count; ++index)
		{
			std::fprintf(stdout, "%s\t%s\n", definitions[index]->name, definitions[index]->title);
		}
		return 0;
	}
	if (output_path == nullptr)
	{
		PrintUsage();
		return 2;
	}
	if (definition_count == 0u)
	{
		std::fputs("該当するベンチが無い (--filter を確かめること)\n", stderr);
		return 2;
	}

	//	初期化の順は core/test/main.cpp・nox::EntryPoint と揃える。
	//	ただしメモリプロファイラは切る。有効だと確保のたびにスタックを採取し、確保の多いベンチの時間が大きく歪む。
	nox::memory::Initialize(std::numeric_limits<nox::int32>::max(), false);
	nox::reflection::Initialize();

	const int exit_code = nox::bench::RunBenchmarks(
		std::span<const nox::bench::Definition* const>(definitions.data(), definition_count),
		options,
		output_path);

	nox::reflection::Finalize();
	nox::memory::ReleaseBootMemory();
	nox::memory::Finialize();	//	綴りは Finialize (nox_memory.h)。これ以降の確保はアサートになる

	return exit_code;
}
