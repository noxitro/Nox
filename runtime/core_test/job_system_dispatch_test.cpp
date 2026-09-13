//	Copyright (C) 2026 NOX ENGINE All rights reserved.

///	@file	job_system_dispatch_test.cpp
///	@brief	nox::JobSystem の Dispatch / Wait が、ワーカーを起こす数を絞っても取りこぼさないことの確認。
///	@details	Dispatch は積めたジョブの本数しかワーカーを起こさない。
///				起こし損ねてもワーカーは queue_mutex_ を取った上で head_ == tail_ を
///				再確認してから眠るので取りこぼさない、というのが設計上の根拠だが、
///				根拠だけでは証拠にならないのでここで実際に回す。
///
///				ジョブ本数 < ワーカー数 の組み合わせを重点的に踏む。
///				WakeAll から本数ぶんの Wake へ変えたときに壊れるとしたらそこなので。

#include	"pch.h"

#include	"../kernel/job_system.h"

#include	<atomic>

namespace
{
	std::atomic<nox::uint32> g_executed_count{ 0u };

	void CountUpJob(void*)noexcept
	{
		g_executed_count.fetch_add(1u, std::memory_order_acq_rel);
	}
}

//	ジョブ本数がワーカー数より少ない状況を繰り返しても、全ジョブが必ず実行される。
TEST(JobSystemDispatch, EveryJobRunsWhenFewerJobsThanWorkers)
{
	static constexpr nox::uint32 k_worker_count = 8u;
	static constexpr nox::uint32 k_iteration_count = 200u;
	static constexpr nox::uint32 k_job_count = 3u;	//	わざとワーカー数より少なくする

	nox::JobSystem job_system;
	job_system.Initialize(k_worker_count);
	ASSERT_EQ(job_system.GetWorkerCount(), k_worker_count);

	g_executed_count.store(0u, std::memory_order_release);

	std::array<nox::Job, k_job_count> jobs{};
	for (nox::Job& job : jobs)
	{
		job = nox::Job{ .func = &CountUpJob, .context = nullptr };
	}

	for (nox::uint32 iteration = 0u; iteration < k_iteration_count; ++iteration)
	{
		nox::JobCounter counter{ 0u };
		job_system.Dispatch(std::span<const nox::Job>(jobs.data(), jobs.size()), counter);
		job_system.Wait(counter);

		//	Wait から戻った時点で counter は 0 でなければならない。
		ASSERT_EQ(counter.load(std::memory_order_acquire), 0u) << "iteration=" << iteration;
	}

	job_system.Finalize();

	EXPECT_EQ(g_executed_count.load(std::memory_order_acquire), k_iteration_count * k_job_count);
}

//	ジョブ本数がワーカー数以上の場合 (WakeAll を通る側) も同じく取りこぼさない。
TEST(JobSystemDispatch, EveryJobRunsWhenMoreJobsThanWorkers)
{
	static constexpr nox::uint32 k_worker_count = 4u;
	static constexpr nox::uint32 k_iteration_count = 100u;
	static constexpr nox::uint32 k_job_count = 16u;

	nox::JobSystem job_system;
	job_system.Initialize(k_worker_count);

	g_executed_count.store(0u, std::memory_order_release);

	std::array<nox::Job, k_job_count> jobs{};
	for (nox::Job& job : jobs)
	{
		job = nox::Job{ .func = &CountUpJob, .context = nullptr };
	}

	for (nox::uint32 iteration = 0u; iteration < k_iteration_count; ++iteration)
	{
		nox::JobCounter counter{ 0u };
		job_system.Dispatch(std::span<const nox::Job>(jobs.data(), jobs.size()), counter);
		job_system.Wait(counter);
		ASSERT_EQ(counter.load(std::memory_order_acquire), 0u) << "iteration=" << iteration;
	}

	job_system.Finalize();

	EXPECT_EQ(g_executed_count.load(std::memory_order_acquire), k_iteration_count * k_job_count);
}

//	ワーカー0本 (--serial-updater 相当) では呼び出しスレッドで実行される。
TEST(JobSystemDispatch, RunsOnCallingThreadWithoutWorkers)
{
	nox::JobSystem job_system;
	job_system.Initialize(0u);
	EXPECT_EQ(job_system.GetWorkerCount(), 0u);

	g_executed_count.store(0u, std::memory_order_release);

	const std::array<nox::Job, 4u> jobs{
		nox::Job{ .func = &CountUpJob, .context = nullptr },
		nox::Job{ .func = &CountUpJob, .context = nullptr },
		nox::Job{ .func = &CountUpJob, .context = nullptr },
		nox::Job{ .func = &CountUpJob, .context = nullptr },
	};

	nox::JobCounter counter{ 0u };
	job_system.Dispatch(std::span<const nox::Job>(jobs.data(), jobs.size()), counter);
	job_system.Wait(counter);

	job_system.Finalize();

	EXPECT_EQ(g_executed_count.load(std::memory_order_acquire), 4u);
}
