//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	job_system.h
///	@brief	常駐ワーカープールへ関数ポインタを配る最小のジョブシステム。
///	@details SOL-AVESの方針そのまま:
///          - ジョブは「関数ポインタ + コンテキスト」だけ。std::functionもヒープも使わない。
///          - 待つ側は遊ばない。Waitに入ったスレッドはキューに残ったジョブを自分で実行する。
///          - ワーカー0本でも必ず動く。その場合は呼び出しスレッド上で全部インライン実行される。
#pragma once
#include	<array>
#include	<atomic>
#include	<limits>
#include	<span>

#include	"basic_definition.h"
#include	"basic_type.h"
#include	"os/mutex.h"
#include	"os/thread.h"
#include	"os/windows.h"

namespace nox
{
	/// @brief ジョブ1つ。仮想関数もstd::functionも持たない。
	struct Job final
	{
		void (*func)(void* context) = nullptr;
		void* context = nullptr;
	};

	/// @brief 未完了ジョブ数。Dispatchで増え、ジョブ完了で減る。
	using JobCounter = std::atomic<nox::uint32>;

	/// @brief 常駐ワーカープール。
	class JobSystem final
	{
	public:
		/// @brief ワーカー数の上限。
		static constexpr nox::uint32 k_max_worker_count = 63u;

		/// @brief キュー容量。2のべき乗。
		/// @details 配分はレイヤー単位(1フレームで数回)であってエンティティ単位ではないので、
		///          ロックフリーdequeまで作り込む必要がない。ここはmutex付きリングで十分。
		///          容量はInitializeで確保済みの固定長配列で、実行時のヒープ確保は一切ない。
		static constexpr nox::uint32 k_job_queue_capacity = 4096u;
		static constexpr nox::uint32 k_job_queue_mask = k_job_queue_capacity - 1u;
		static_assert((k_job_queue_capacity& k_job_queue_mask) == 0u, "容量は2のべき乗である必要があります");

		/// @brief ワーカー以外のスレッドが返すインデックス。
		static constexpr nox::uint32 k_invalid_worker_index = std::numeric_limits<nox::uint32>::max();

		JobSystem()noexcept;
		~JobSystem();

		JobSystem(const JobSystem&) = delete;
		JobSystem(JobSystem&&) = delete;
		JobSystem& operator=(const JobSystem&) = delete;

		/// @brief ワーカーを起動する。worker_countは[0, k_max_worker_count]へ丸められる。
		/// @details 0を指定した場合はスレッドを1本も作らず、全ジョブがDispatchの呼び出し元で走る。
		void Initialize(nox::uint32 worker_count);

		/// @brief ワーカーを停止して回収する。二重呼び出しは無害。
		void Finalize();

		/// @brief 既定のワーカー数(論理プロセッサ数 - 1)。
		[[nodiscard]] static nox::uint32 GetDefaultWorkerCount()noexcept;

		[[nodiscard]] inline nox::uint32 GetWorkerCount()const noexcept { return worker_count_; }

		/// @brief ジョブ群を配る。counterはjobs.size()だけ増える。
		/// @details ワーカー0本ならこの場で全部実行して戻る。
		void Dispatch(std::span<const nox::Job> jobs, nox::JobCounter& counter);

		/// @brief counterが0になるまで待つ。待っている間、キューに残ったジョブを自分で実行する。
		void Wait(nox::JobCounter& counter);

		/// @brief 現在のスレッドがこのプールのワーカーか。
		[[nodiscard]] static bool IsWorkerThread()noexcept;

		/// @brief 現在のスレッドのワーカー番号。ワーカーでなければk_invalid_worker_index。
		[[nodiscard]] static nox::uint32 GetWorkerIndex()noexcept;

	private:
		/// @brief キューに積まれたジョブ。完了時にどのcounterを下げるかを持つ。
		struct QueuedJob final
		{
			nox::Job job{};
			nox::JobCounter* counter = nullptr;
		};

		/// @brief キューから1つ取り出す。空ならfalse。
		[[nodiscard]] bool TryPopJob(nox::JobSystem::QueuedJob& out)noexcept;

		/// @brief ジョブを実行してcounterを下げる。
		static void RunJob(const nox::JobSystem::QueuedJob& queued)noexcept;

		void WorkerMain(nox::uint32 worker_index);

	private:
		/// @brief 固定長リング。Initialize後にサイズが変わることはない。
		std::array<nox::JobSystem::QueuedJob, k_job_queue_capacity> queue_;

		/// @brief 次に取り出す位置(単調増加)。
		nox::uint32 head_;
		/// @brief 次に積む位置(単調増加)。
		nox::uint32 tail_;

		mutable nox::os::Mutex queue_mutex_;
		::CONDITION_VARIABLE job_available_;

		std::array<nox::os::Thread, k_max_worker_count> workers_;

		nox::uint32 worker_count_;
		std::atomic<bool> quit_;
		bool initialized_;

		/// @brief 自スレッドのワーカー番号。ワーカー以外はk_invalid_worker_index。
		static inline thread_local constinit nox::uint32 local_worker_index_ = k_invalid_worker_index;
	};
}
