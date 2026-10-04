//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	bench_job.cpp
///	@brief	nox::JobSystem のベンチマーク
///	@details	UpdaterGraph はレイヤーごと・Chunk の束ごとに Dispatch + Wait を回すので、
///				その往復コストはフレームごとに効く。job_system.cpp のコメントにある通り、
///				ワーカーの起こし方ひとつで 0.65µs が 27µs になった過去があるので、推移を残しておく。
///
///				ワーカー数は nox::JobSystem::GetDefaultWorkerCount() (論理 CPU 数 - 1) に揃える。
///				Wait する本スレッドも自分でジョブを引くので、仕事をするスレッドはワーカー数 + 1。
///				Dispatch / Wait はヒープ確保をしない設計 (job_system.h) なので予算は 0。

#include	"pch.h"
#include	"bench.h"

namespace
{
	void EmptyJob(void*)noexcept
	{
	}

	/// @brief 空ジョブを job_count 個配って待つ
	template<nox::uint32 JobCount>
	void RunDispatchWait(nox::bench::State& state, const nox::uint32 worker_count)
	{
		std::array<nox::Job, JobCount> jobs{};
		for (nox::Job& job : jobs)
		{
			job = nox::Job{ .func = &EmptyJob, .context = nullptr };
		}

		nox::JobSystem job_system;
		job_system.Initialize(worker_count);
		state.SetThreads(job_system.GetWorkerCount() + 1u);

		state.Run([&job_system, &jobs](const nox::uint64 op_count)
			{
				for (nox::uint64 op = 0u; op < op_count; ++op)
				{
					nox::JobCounter counter{ 0u };
					job_system.Dispatch(std::span<const nox::Job>(jobs.data(), jobs.size()), counter);
					job_system.Wait(counter);
				}
			});

		job_system.Finalize();
	}

	void BenchDispatchWait4(nox::bench::State& state)
	{
		RunDispatchWait<4u>(state, nox::JobSystem::GetDefaultWorkerCount());
	}

	void BenchDispatchWait64(nox::bench::State& state)
	{
		RunDispatchWait<64u>(state, nox::JobSystem::GetDefaultWorkerCount());
	}

	/// @brief ワーカー 0 本 (その場で実行) の Dispatch + Wait。スレッドをまたがない下限
	void BenchDispatchInline64(nox::bench::State& state)
	{
		RunDispatchWait<64u>(state, 0u);
	}

	/// @brief 小さな計算ジョブ 1 個分の文脈
	struct SpinJobContext final
	{
		std::atomic<nox::uint64>* sink;
		nox::uint32 work;
	};

	void SpinJob(void* const context)noexcept
	{
		const SpinJobContext* const job_context = static_cast<const SpinJobContext*>(context);
		nox::uint64 value = 0u;
		for (nox::uint32 index = 0u; index < job_context->work; ++index)
		{
			value += static_cast<nox::uint64>(index) * 2654435761ull;
		}
		job_context->sink->fetch_add(value, std::memory_order_relaxed);
	}

	/// @brief 1024 個の小さなジョブを配る。Chunk 単位の並列列挙と同じ粒度の配り方
	void BenchFanout1024(nox::bench::State& state)
	{
		static constexpr nox::uint32 kJobCount = 1024u;	//	キュー容量 (4096) を超えないこと

		std::atomic<nox::uint64> sink{ 0u };
		SpinJobContext context{ .sink = &sink, .work = 256u };
		std::array<nox::Job, kJobCount> jobs{};
		for (nox::Job& job : jobs)
		{
			job = nox::Job{ .func = &SpinJob, .context = &context };
		}

		nox::JobSystem job_system;
		job_system.Initialize(nox::JobSystem::GetDefaultWorkerCount());
		state.SetThreads(job_system.GetWorkerCount() + 1u);
		state.SetItemsPerOp(kJobCount);

		state.Run([&job_system, &jobs](const nox::uint64 op_count)
			{
				for (nox::uint64 op = 0u; op < op_count; ++op)
				{
					nox::JobCounter counter{ 0u };
					job_system.Dispatch(std::span<const nox::Job>(jobs.data(), jobs.size()), counter);
					job_system.Wait(counter);
				}
			});

		job_system.Finalize();
		nox::bench::DoNotOptimize(sink);
	}
}

std::span<const nox::bench::Definition> nox::bench::GetJobBenchmarks()noexcept
{
	static constexpr std::array<nox::bench::Definition, 4> kDefinitions{ {
		{ .name = "job/dispatch_wait/4", .title = "空ジョブ 4 個の Dispatch + Wait", .per = "dispatch", .alloc_budget = 0, .function = &BenchDispatchWait4 },
		{ .name = "job/dispatch_wait/64", .title = "空ジョブ 64 個の Dispatch + Wait", .per = "dispatch", .alloc_budget = 0, .function = &BenchDispatchWait64 },
		{ .name = "job/dispatch_inline/64", .title = "ワーカー 0 (その場で実行) の Dispatch + Wait", .per = "dispatch", .alloc_budget = 0, .function = &BenchDispatchInline64 },
		{ .name = "job/fanout/1024", .title = "小さな計算ジョブ 1024 個の分配", .per = "job", .alloc_budget = 0, .function = &BenchFanout1024 },
	} };
	return kDefinitions;
}
