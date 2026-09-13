//	Copyright (C) 2026 NOX ENGINE All rights reserved.

///	@file	job_system.cpp
///	@brief	job_system
#include	"pch.h"
#include	"job_system.h"

#include	"assertion.h"
#include	"log_id.h"
#include	"log_trace.h"
#include	"os/os_utility.h"

namespace
{
	/// @brief キューが空だったときに、ブロックへ移行する前に回す空回しの回数。
	/// @details 直後にジョブが積まれるケースを条件変数の往復なしで拾うための短いスピン。
	constexpr nox::uint32 k_job_spin_count = 64u;

	/// @brief ワーカースレッド名の最大長("JobWorker#63" + 終端)。
	constexpr size_t k_worker_thread_name_capacity = 32u;

	void make_worker_thread_name(std::array<nox::char16, k_worker_thread_name_capacity>& dest, const nox::uint32 index)noexcept
	{
		constexpr std::u16string_view prefix = u"JobWorker#";
		size_t length = 0u;
		for (const nox::char16 character : prefix)
		{
			dest[length++] = character;
		}

		if (index >= 10u)
		{
			dest[length++] = static_cast<nox::char16>(u'0' + ((index / 10u) % 10u));
		}
		dest[length++] = static_cast<nox::char16>(u'0' + (index % 10u));
		dest[length] = u'\0';
	}
}

nox::JobSystem::JobSystem()noexcept :
	queue_{},
	head_(0u),
	tail_(0u),
	queue_mutex_(),
	job_available_(),
	workers_{},
	worker_count_(0u),
	quit_(false),
	initialized_(false)
{
	::InitializeConditionVariable(&job_available_);
}

nox::JobSystem::~JobSystem()
{
	Finalize();
}

nox::uint32 nox::JobSystem::GetDefaultWorkerCount()noexcept
{
	const nox::uint32 processor_count = nox::os::GetLogicalProcessorCount();

	//	メインスレッド(ゲームスレッド)自身がWaitでジョブを手伝うので、その分を引く。
	const nox::uint32 worker_count = (processor_count > 1u) ? (processor_count - 1u) : 0u;
	return std::min(worker_count, k_max_worker_count);
}

void nox::JobSystem::Initialize(const nox::uint32 worker_count)
{
	if (initialized_)
	{
		NOX_ASSERT(false, u8"JobSystemが二重にInitializeされました");
		return;
	}

	head_ = 0u;
	tail_ = 0u;
	quit_.store(false, std::memory_order_release);
	worker_count_ = std::min(worker_count, k_max_worker_count);
	initialized_ = true;

	for (nox::uint32 index = 0u; index < worker_count_; ++index)
	{
		//	std::functionの確保はここ(初期化時)だけ。以降のディスパッチ経路には一切確保がない。
		workers_[index].Dispatch([this, index]()
			{
				WorkerMain(index);
			});
	}
}

void nox::JobSystem::Finalize()
{
	if (initialized_ == false)
	{
		return;
	}

	//	ワーカーはキューmutexの下でquit_を判定してから眠るので、こちらも同じmutexの下で立てる。
	//	そうしないと「判定した直後・眠る直前」にWakeAllを撃ってしまい、起こし損ねて永久に眠る。
	queue_mutex_.Lock();
	quit_.store(true, std::memory_order_release);
	queue_mutex_.Unlock();
	::WakeAllConditionVariable(&job_available_);

	for (nox::uint32 index = 0u; index < worker_count_; ++index)
	{
		workers_[index].Wait();
	}

	worker_count_ = 0u;
	head_ = 0u;
	tail_ = 0u;
	initialized_ = false;
}

bool nox::JobSystem::IsWorkerThread()noexcept
{
	return local_worker_index_ != k_invalid_worker_index;
}

nox::uint32 nox::JobSystem::GetWorkerIndex()noexcept
{
	return local_worker_index_;
}

void nox::JobSystem::RunJob(const nox::JobSystem::QueuedJob& queued)noexcept
{
	if (queued.job.func != nullptr)
	{
		queued.job.func(queued.job.context);
	}

	if (queued.counter != nullptr)
	{
		queued.counter->fetch_sub(1u, std::memory_order_acq_rel);
	}
}

bool nox::JobSystem::TryPopJob(nox::JobSystem::QueuedJob& out)noexcept
{
	queue_mutex_.Lock();
	const bool has_job = (head_ != tail_);
	if (has_job)
	{
		out = queue_[head_ & k_job_queue_mask];
		++head_;
	}
	queue_mutex_.Unlock();
	return has_job;
}

void nox::JobSystem::Dispatch(const std::span<const nox::Job> jobs, nox::JobCounter& counter)
{
	if (jobs.empty())
	{
		return;
	}

	//	ワーカーが居ないなら、そのままこのスレッドで実行する。counterに触る必要すらない。
	if (worker_count_ == 0u || initialized_ == false)
	{
		for (const nox::Job& job : jobs)
		{
			if (job.func != nullptr)
			{
				job.func(job.context);
			}
		}
		return;
	}

	//	どのワーカーよりも先にcounterを積む。積み終わる前に0を観測させないため。
	counter.fetch_add(static_cast<nox::uint32>(jobs.size()), std::memory_order_acq_rel);

	size_t enqueued_count = 0u;
	queue_mutex_.Lock();
	for (const nox::Job& job : jobs)
	{
		if ((tail_ - head_) >= k_job_queue_capacity)
		{
			break;
		}

		queue_[tail_ & k_job_queue_mask] = nox::JobSystem::QueuedJob{ .job = job, .counter = &counter };
		++tail_;
		++enqueued_count;
	}
	queue_mutex_.Unlock();

	//	起こすのは積めたジョブの本数まで。WakeAll だと 4 本積んだだけでもワーカー全員が
	//	起きて、大半が「何も無い」と分かって寝直す(thundering herd)。
	//	空ジョブ 4 本の Dispatch + Wait が 4 ワーカーで 0.65us、31 ワーカーで 27us まで
	//	膨らんでいた原因がこれ。起こし損ねても取りこぼさない: ワーカーは
	//	queue_mutex_ を取った上で head_ == tail_ を再確認してから眠る。
	//	Wait も条件変数では眠らない(YieldProcessor で回る)ので待ち手が取り残されることもない。
	if (enqueued_count >= static_cast<size_t>(worker_count_))
	{
		::WakeAllConditionVariable(&job_available_);
	}
	else
	{
		for (size_t index = 0u; index < enqueued_count; ++index)
		{
			::WakeConditionVariable(&job_available_);
		}
	}

	if (enqueued_count != jobs.size())
	{
		//	容量を超えた分は取りこぼさずこの場で実行する。設計上ここへ来たら容量が足りていない。
		NOX_ASSERT(false, u8"JobSystemのキューが溢れました 容量={0} 要求={1}",
			k_job_queue_capacity, static_cast<nox::uint32>(jobs.size()));

		for (size_t index = enqueued_count; index < jobs.size(); ++index)
		{
			RunJob(nox::JobSystem::QueuedJob{ .job = jobs[index], .counter = &counter });
		}
	}
}

void nox::JobSystem::Wait(nox::JobCounter& counter)
{
	//	待つ側も働く。メインスレッドを遊ばせたままキューにジョブが残る状態を作らない。
	nox::JobSystem::QueuedJob queued;
	while (counter.load(std::memory_order_acquire) != 0u)
	{
		if (TryPopJob(queued))
		{
			RunJob(queued);
			continue;
		}

		//	キューは空だが未完了のジョブがある(他スレッドが実行中)。完了を待つだけ。
		::YieldProcessor();
	}
}

void nox::JobSystem::WorkerMain(const nox::uint32 worker_index)
{
	local_worker_index_ = worker_index;

	std::array<nox::char16, k_worker_thread_name_capacity> thread_name{};
	make_worker_thread_name(thread_name, worker_index);
	nox::os::Thread::SetThreadName(std::u16string_view(thread_name.data()));

	nox::JobSystem::QueuedJob queued;
	while (quit_.load(std::memory_order_acquire) == false)
	{
		if (TryPopJob(queued))
		{
			RunJob(queued);
			continue;
		}

		//	すぐ次が積まれる可能性が高いので、条件変数へ落ちる前に少しだけ空回しする。
		bool found = false;
		for (nox::uint32 spin = 0u; spin < k_job_spin_count; ++spin)
		{
			::YieldProcessor();
			if (TryPopJob(queued))
			{
				found = true;
				break;
			}
		}

		if (found)
		{
			RunJob(queued);
			continue;
		}

		//	本当に何も無いのでブロックする。スピンし続けて他のコアを焼かない。
		queue_mutex_.Lock();
		while (head_ == tail_ && quit_.load(std::memory_order_acquire) == false)
		{
			::SleepConditionVariableCS(
				&job_available_,
				const_cast<::CRITICAL_SECTION*>(&queue_mutex_.GetCriticalSection()),
				INFINITE);
		}
		queue_mutex_.Unlock();
	}

	local_worker_index_ = k_invalid_worker_index;
}
