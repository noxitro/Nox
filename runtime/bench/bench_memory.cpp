//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	bench_memory.cpp
///	@brief	nox::memory のベンチマーク
///	@details	エンジンの確保はすべて nox::memory::Allocate (malloc + 管理ヘッダ + 全体ロック) を通る。
///				1 回あたりの値段を CRT の malloc と並べて見られるようにしておき、
///				「確保を 1 回減らすと何 ns 浮くか」の目安にする。

#include	"pch.h"
#include	"bench.h"

namespace
{
	/// @brief nox::memory::Allocate + Deallocate を size バイトで繰り返す
	void RunNoxAllocFree(nox::bench::State& state, const size_t size)
	{
		state.Run([size](const nox::uint64 op_count)
			{
				for (nox::uint64 op = 0u; op < op_count; ++op)
				{
					void* const pointer = nox::memory::Allocate(size, nox::memory::InstanceType::Other);
					nox::bench::DoNotOptimize(pointer);
					nox::memory::Deallocate(pointer);
				}
			});
	}

	void BenchNoxAllocFree64(nox::bench::State& state)
	{
		RunNoxAllocFree(state, 64u);
	}

	void BenchNoxAllocFree4096(nox::bench::State& state)
	{
		RunNoxAllocFree(state, 4096u);
	}

	/// @brief グローバル operator new[] / delete[] (kernel の new_delete.h 経由で nox::memory へ流れる)
	void BenchNewDelete64(nox::bench::State& state)
	{
		state.Run([](const nox::uint64 op_count)
			{
				for (nox::uint64 op = 0u; op < op_count; ++op)
				{
					std::byte* const pointer = new std::byte[64];
					//	ポインタを外へ逃がしておかないと、対になった new / delete ごと消されることがある
					nox::bench::DoNotOptimize(pointer);
					delete[] pointer;
				}
			});
	}

	/// @brief 比較用: CRT の malloc / free。nox::memory を通らないので確保回数には数えられない
	void BenchCrtMallocFree64(nox::bench::State& state)
	{
		state.Run([](const nox::uint64 op_count)
			{
				for (nox::uint64 op = 0u; op < op_count; ++op)
				{
					void* const pointer = std::malloc(64u);
					nox::bench::DoNotOptimize(pointer);
					std::free(pointer);
				}
			});
	}
}

std::span<const nox::bench::Definition> nox::bench::GetMemoryBenchmarks()noexcept
{
	static constexpr std::array<nox::bench::Definition, 4> kDefinitions{ {
		{ .name = "memory/nox_alloc_free/64", .title = "nox::memory::Allocate + Deallocate (64 B)", .per = "pair", .alloc_budget = 1, .function = &BenchNoxAllocFree64 },
		{ .name = "memory/nox_alloc_free/4096", .title = "nox::memory::Allocate + Deallocate (4 KiB)", .per = "pair", .alloc_budget = 1, .function = &BenchNoxAllocFree4096 },
		{ .name = "memory/new_delete/64", .title = "グローバル operator new + delete (64 B)", .per = "pair", .alloc_budget = 1, .function = &BenchNewDelete64 },
		{ .name = "memory/crt_malloc_free/64", .title = "比較用: CRT の malloc + free (64 B, 計数対象外)", .per = "pair", .alloc_budget = 0, .function = &BenchCrtMallocFree64 },
	} };
	return kDefinitions;
}
