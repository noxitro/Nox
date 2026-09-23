// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

/// @file	job_system_test.cpp
/// @brief	JobSystemのセルフテスト。配分・完了待ち・並列性を起動時に検証する。
#include "pch.h"

#include "../../kernel/kernel.h"
#include "../../reflection/reflection.h"
#include "test.h"

#include "../../kernel/job_system.h"
#include "../../kernel/assertion.h"
#include "../../kernel/stop_watch.h"
#include "../log_id.h"

namespace
{
	/// @brief 単純に加算するだけのジョブ。ジョブ本体は関数ポインタなのでキャプチャは持てない。
	struct CountUpJobContext final
	{
		std::atomic<nox::uint32>* counter = nullptr;
	};

	void count_up_job(void* const context)noexcept
	{
		auto* const job_context = static_cast<CountUpJobContext*>(context);
		job_context->counter->fetch_add(1u, std::memory_order_acq_rel);
	}

	/// @brief 指定ミリ秒だけCPUを回し続けるジョブ。Sleepでは並列性を測れないのでビジーで回す。
	struct BusySpinJobContext final
	{
		nox::uint32 spin_milli_seconds = 0u;
		std::atomic<nox::uint64>* accumulator = nullptr;
	};

	void busy_spin_job(void* const context)noexcept
	{
		auto* const job_context = static_cast<BusySpinJobContext*>(context);

		nox::StopWatch stop_watch;
		stop_watch.Start();

		nox::uint64 work = 0ull;
		while (stop_watch.ElapsedMilliseconds() < static_cast<nox::float_t>(job_context->spin_milli_seconds))
		{
			//	最適化で消えないように結果を積む。
			for (nox::uint32 index = 0u; index < 1024u; ++index)
			{
				work += index * 2654435761ull;
			}
		}

		job_context->accumulator->fetch_add(work, std::memory_order_relaxed);
	}

	/// @brief 10000ジョブを配ってWaitし、全部完了しているかを見る。
	void TestJobSystemDispatchAndWait()
	{
		static constexpr nox::uint32 k_job_count = 10000u;

		nox::JobSystem job_system;
		job_system.Initialize(nox::JobSystem::GetDefaultWorkerCount());

		std::atomic<nox::uint32> executed_count{ 0u };
		CountUpJobContext job_context{ .counter = &executed_count };

		//	1回のDispatchはキュー容量(4096)を超えられないので、チャンクに割って配る。
		//	チャンクごとにWaitして畳むことで、未処理ジョブがキュー容量を超えることがない。
		static constexpr nox::uint32 k_chunk_size = 1024u;
		std::array<nox::Job, k_chunk_size> jobs{};
		for (nox::Job& job : jobs)
		{
			job = nox::Job{ .func = &count_up_job, .context = &job_context };
		}

		nox::JobCounter counter{ 0u };
		for (nox::uint32 dispatched = 0u; dispatched < k_job_count; dispatched += k_chunk_size)
		{
			const nox::uint32 count = std::min(k_chunk_size, k_job_count - dispatched);
			job_system.Dispatch(std::span<const nox::Job>(jobs.data(), count), counter);
			job_system.Wait(counter);
		}

		NOX_ASSERT(counter.load(std::memory_order_acquire) == 0u,
			u8"JobSystem::Waitから抜けたのにJobCounterが0ではありません: {0}",
			counter.load(std::memory_order_acquire));

		NOX_ASSERT(executed_count.load(std::memory_order_acquire) == k_job_count,
			u8"JobSystemのジョブ実行数が一致しません 期待={0} 実際={1}",
			k_job_count, executed_count.load(std::memory_order_acquire));

		job_system.Finalize();
	}

	/// @brief ワーカー0本 と 既定ワーカー数 で同じ仕事量にかかる時間を測る。
	/// @details コア数は環境で変わるので、アサートは「並列 <= 直列」だけ。実測値はログへ出す。
	void TestJobSystemParallelSpeedup()
	{
		static constexpr nox::uint32 k_spin_job_count = 8u;
		static constexpr nox::uint32 k_spin_milli_seconds = 20u;

		std::atomic<nox::uint64> accumulator{ 0ull };
		BusySpinJobContext job_context{
			.spin_milli_seconds = k_spin_milli_seconds,
			.accumulator = &accumulator,
		};

		std::array<nox::Job, k_spin_job_count> jobs{};
		for (nox::Job& job : jobs)
		{
			job = nox::Job{ .func = &busy_spin_job, .context = &job_context };
		}

		const auto measure = [&jobs](const nox::uint32 worker_count) -> nox::float_t
			{
				nox::JobSystem job_system;
				job_system.Initialize(worker_count);

				nox::JobCounter counter{ 0u };
				nox::StopWatch stop_watch;
				stop_watch.Start();

				job_system.Dispatch(std::span<const nox::Job>(jobs), counter);
				job_system.Wait(counter);

				const nox::float_t elapsed = stop_watch.ElapsedMilliseconds();
				job_system.Finalize();
				return elapsed;
			};

		const nox::uint32 default_worker_count = nox::JobSystem::GetDefaultWorkerCount();
		const nox::float_t serial_milli_seconds = measure(0u);
		const nox::float_t parallel_milli_seconds = measure(default_worker_count);

		NOX_INFO_LINE(nox::log_id::CoreCommon,
			u8"JobSystem計測: ジョブ数={0} 1ジョブ={1}ms 直列(ワーカー0)={2}ms 並列(ワーカー{3})={4}ms",
			k_spin_job_count,
			k_spin_milli_seconds,
			serial_milli_seconds,
			default_worker_count,
			parallel_milli_seconds);

		//	コア数は環境依存なので速度比までは要求しない。遅くなっていないことだけを見る。
		//	ワーカー0本のときは直列そのものなので、そのケースは比較対象にしない。
		if (default_worker_count > 0u)
		{
			NOX_ASSERT(parallel_milli_seconds <= serial_milli_seconds,
				u8"並列実行が直列実行より遅くなっています 直列={0}ms 並列={1}ms",
				serial_milli_seconds, parallel_milli_seconds);
		}

		NOX_ASSERT(accumulator.load(std::memory_order_acquire) != 0ull,
			u8"ビジースピンジョブが実行されていません");
	}

	/// @brief ワーカー0本でも全ジョブがその場で走ること。
	void TestJobSystemInlineFallback()
	{
		nox::JobSystem job_system;
		job_system.Initialize(0u);

		NOX_ASSERT(job_system.GetWorkerCount() == 0u, u8"ワーカー数0を指定したのに0になっていません");
		NOX_ASSERT(nox::JobSystem::IsWorkerThread() == false, u8"呼び出しスレッドがワーカー扱いになっています");
		NOX_ASSERT(nox::JobSystem::GetWorkerIndex() == nox::JobSystem::k_invalid_worker_index,
			u8"ワーカーでないスレッドが有効なワーカー番号を返しています");

		std::atomic<nox::uint32> executed_count{ 0u };
		CountUpJobContext job_context{ .counter = &executed_count };

		std::array<nox::Job, 32u> jobs{};
		for (nox::Job& job : jobs)
		{
			job = nox::Job{ .func = &count_up_job, .context = &job_context };
		}

		nox::JobCounter counter{ 0u };
		job_system.Dispatch(std::span<const nox::Job>(jobs), counter);
		job_system.Wait(counter);

		NOX_ASSERT(executed_count.load(std::memory_order_acquire) == jobs.size(),
			u8"ワーカー0本のインライン実行でジョブが取りこぼされました 期待={0} 実際={1}",
			static_cast<nox::uint32>(jobs.size()), executed_count.load(std::memory_order_acquire));

		job_system.Finalize();
	}
}

void nox::test::TestJobSystem()
{
	TestJobSystemInlineFallback();
	TestJobSystemDispatchAndWait();
	TestJobSystemParallelSpeedup();
}
