//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	bench.h
///	@brief	ベンチマークの計測ハーネス
///	@details	1 本のベンチマークは「準備 → state.Run(body) → 後片付け」の関数として書く。
///				body(n) は「1 op を n 回」実行する関数オブジェクト。計測は Run の中で次の順に行う。
///
///				  1. 準備運転: body(1) を 1 回 (遅延初期化・容量の伸長を計測から外す)
///				  2. 校正    : 1 サンプルが min_sample_ns 以上になるまで op 回数を増やす
///				               (QueryPerformanceCounter の分解能は仮想マシンでは 100ns なので、
///				                短い区間を測ると量子化誤差が支配的になる)
///				  3. サンプル: 決めた op 回数で sample_count 回測り、1 op あたりの ns を記録する
///				  4. 確保計数: 固定の op 回数で 2 回実行し、nox::memory の確保回数を数える
///				               (2 回が一致すれば「定常状態で決定的」とみなす)
///
///				時間はマシンの負荷で揺れるので CI では判定に使わず報告だけにする。
///				確保回数は揺れないので、予算 (alloc_budget) を超えたら終了コード 3 で CI を落とす。
#pragma once

namespace nox::bench
{
	class State;

	/// @brief ベンチマーク 1 本の本体
	using BenchFunction = void(*)(nox::bench::State& state);

	/// @brief 確保回数の予算を設けないことを表す値
	inline constexpr nox::int32 kNoBudget = -1;

	/// @brief ベンチマーク 1 本の定義
	/// @details 名前はサイト・履歴・CI の要約をつなぐキーなので、一度決めたら変えない。
	struct Definition final
	{
		/// @brief "グループ/名前[/条件]" (小文字の ASCII)。先頭の区切りまでがグループになる
		const char* name;

		/// @brief 日本語の短い説明 (UTF-8)
		const char* title;

		/// @brief 1 op が何を表すか ("entity" / "dispatch" / "call" など)
		const char* per;

		/// @brief 1 op あたりの確保回数の上限。kNoBudget なら判定しない
		nox::int32 alloc_budget;

		/// @brief 本体
		nox::bench::BenchFunction function;
	};

	/// @brief コマンドラインから決まる実行条件
	struct Options final
	{
		/// @brief 1 サンプルの最低時間 [ns]
		double min_sample_ns = 2.0e6;

		/// @brief 1 本あたりのサンプル数
		nox::uint32 sample_count = 11u;

		/// @brief 確保計数に使う op 回数 (固定。伸長の償却が回数で変わらないようにする)
		nox::uint64 alloc_ops = 32u;

		/// @brief 動作確認だけ行う (各 op を 1 回ずつ。Debug 構成向け)
		bool smoke = false;

		/// @brief 計測スレッドを CPU 1 に固定する (--no-pin で外す。固定の有無で結果を比べるため)
		bool pin = true;
	};

	namespace detail
	{
		/// @brief 最適化で値が消されないようにするための受け口 (別 TU で定義し、インライン化させない)
		__declspec(noinline) void UseCharPointer(const volatile char* pointer)noexcept;

		/// @brief 呼び出しスレッドの QueryThreadCycleTime の値
		/// @details 不変 TSC の基準ティックで数えた「スレッドが実際に走っていた量」。コアのクロック数ではない
		///          (ブーストしても増えない)。壁時計と違い、スレッドが止められていた間は進まない。
		[[nodiscard]] nox::uint64 ReadThreadCycles()noexcept;

		/// @brief ベンチの本体を 1 回呼ぶ
		/// @details わざとインライン化させない。本体 (ラムダ) が State::Run の中へ展開されるかどうかは
		///          Run の大きさやベンチごとのコンパイラの判断で変わり、同じ処理でもコード生成が変わって
		///          しまう (参照で捕捉した値がレジスタに載らなくなるなど)。呼び出しはサンプル 1 回につき
		///          1 回なので、この関数呼び出しの費用は計測に影響しない。
		template<class Body>
		__declspec(noinline) void InvokeBody(Body& body, const nox::uint64 op_count)
		{
			body(op_count);
		}
	}

	/// @brief 値を「使った」ことにして、計算そのものを最適化で消させない
	/// @details Google Benchmark の benchmark::DoNotOptimize と同じ方式。
	///          clang-cl は GNU 形式のインラインアセンブリで、MSVC は x64 でインラインアセンブリが
	///          使えないので、インライン化しない関数へアドレスを渡してコンパイラの推論を断つ。
	template<class T>
	inline void DoNotOptimize(const T& value)noexcept
	{
#if defined(__clang__)
		if constexpr (std::is_trivially_copyable_v<T> && (sizeof(T) <= sizeof(T*)))
		{
			__asm__ __volatile__("" : : "r,m"(value) : "memory");
		}
		else
		{
			__asm__ __volatile__("" : : "m"(value) : "memory");
		}
#else
		nox::bench::detail::UseCharPointer(&reinterpret_cast<const volatile char&>(value));
		std::atomic_signal_fence(std::memory_order_seq_cst);
#endif
	}

	/// @brief メモリへの書き込みを、この地点までに済ませたことにする (コンパイラの並べ替え止め)
	inline void ClobberMemory()noexcept
	{
		std::atomic_signal_fence(std::memory_order_seq_cst);
	}

	/// @brief 計測 1 本分の状態と結果
	class State final
	{
	public:
		inline explicit State(const nox::bench::Options& options)noexcept :
			options_(options)
		{
		}

		State(const State&) = delete;
		State& operator=(const State&) = delete;

		/// @brief 1 op で処理する要素数。結果の ns・確保回数はこの数で割った「1 要素あたり」になる
		/// @details 例: 1 万体を 1 回列挙する op なら 10000 を設定すると ns/entity になる。
		inline void SetItemsPerOp(const nox::uint64 items)noexcept
		{
			items_per_op_ = (items == 0u) ? 1u : items;
		}

		/// @brief 仕事をするスレッド数 (JobSystem を使うベンチはワーカー数 + 1 を設定する)
		inline void SetThreads(const nox::uint32 threads)noexcept
		{
			threads_ = (threads == 0u) ? 1u : threads;
		}

		/// @brief 計測本体。1 本のベンチにつき 1 回だけ呼ぶ
		template<class Body>
		void Run(Body&& body);

		[[nodiscard]] inline bool HasRun()const noexcept { return has_run_; }
		[[nodiscard]] inline nox::uint32 GetThreads()const noexcept { return threads_; }
		[[nodiscard]] inline nox::uint64 GetOpsPerSample()const noexcept { return ops_per_sample_; }
		[[nodiscard]] inline std::span<const double> GetSamplesNs()const noexcept { return samples_ns_; }
		[[nodiscard]] inline std::span<const double> GetCyclesPerOp()const noexcept { return cycles_per_op_; }
		[[nodiscard]] inline double GetAllocsPerOp()const noexcept { return allocs_per_op_; }
		[[nodiscard]] inline double GetAllocBytesPerOp()const noexcept { return alloc_bytes_per_op_; }
		[[nodiscard]] inline double GetFreesPerOp()const noexcept { return frees_per_op_; }
		[[nodiscard]] inline bool IsAllocStable()const noexcept { return alloc_stable_; }

	private:
		using Clock = std::chrono::steady_clock;

		/// @brief 校正で 1 サンプルの op 回数をここまでしか増やさない
		static constexpr nox::uint64 kMaxOpsPerSample = nox::uint64{ 1u } << 30;

		[[nodiscard]] static inline double ElapsedNs(const Clock::time_point begin, const Clock::time_point end)noexcept
		{
			return std::chrono::duration<double, std::nano>(end - begin).count();
		}

		const nox::bench::Options& options_;
		nox::uint64 items_per_op_ = 1u;
		nox::uint32 threads_ = 1u;
		bool has_run_ = false;

		nox::uint64 ops_per_sample_ = 0u;
		std::vector<double> samples_ns_;
		std::vector<double> cycles_per_op_;

		double allocs_per_op_ = 0.0;
		double alloc_bytes_per_op_ = 0.0;
		double frees_per_op_ = 0.0;
		bool alloc_stable_ = true;
	};

	/// @brief kernel 系 (コンテナ・文字列・delegate・数学・ロック) のベンチ一覧
	[[nodiscard]] std::span<const nox::bench::Definition> GetKernelBenchmarks()noexcept;

	/// @brief nox::memory のベンチ一覧
	[[nodiscard]] std::span<const nox::bench::Definition> GetMemoryBenchmarks()noexcept;

	/// @brief JobSystem のベンチ一覧
	[[nodiscard]] std::span<const nox::bench::Definition> GetJobBenchmarks()noexcept;

	/// @brief ECS (World / EntitySystem / UpdaterGraph) のベンチ一覧
	[[nodiscard]] std::span<const nox::bench::Definition> GetEcsBenchmarks()noexcept;

	/// @brief 指定したベンチを全部走らせ、結果を JSON で書き出す
	/// @return プロセスの終了コード (0: 成功 / 1: 出力失敗 / 3: 確保回数の予算超過)
	[[nodiscard]] int RunBenchmarks(
		std::span<const nox::bench::Definition* const> definitions,
		const nox::bench::Options& options,
		const char* output_path);
}

template<class Body>
void nox::bench::State::Run(Body&& body)
{
	if (has_run_)
	{
		//	1 本のベンチで 2 回呼ぶのは書き間違い。後の呼び出しは無視して最初の結果を残す
		return;
	}
	has_run_ = true;

	//	1. 準備運転
	nox::bench::detail::InvokeBody(body, nox::uint64{ 1u });

	//	2. 校正
	nox::uint64 ops = 1u;
	if (options_.smoke == false)
	{
		for (;;)
		{
			const Clock::time_point begin = Clock::now();
			nox::bench::detail::InvokeBody(body, ops);
			const Clock::time_point end = Clock::now();
			const double elapsed_ns = ElapsedNs(begin, end);
			if (elapsed_ns >= options_.min_sample_ns || ops >= kMaxOpsPerSample)
			{
				break;
			}

			//	目標を少し超えるように見積もる。見積もりが外れても 2〜100 倍の範囲で詰めていく
			double scale = (elapsed_ns > 0.0) ? ((options_.min_sample_ns * 1.2) / elapsed_ns) : 100.0;
			scale = (scale < 2.0) ? 2.0 : ((scale > 100.0) ? 100.0 : scale);
			const double next_ops = std::ceil(static_cast<double>(ops) * scale);
			ops = (next_ops >= static_cast<double>(kMaxOpsPerSample)) ? kMaxOpsPerSample : static_cast<nox::uint64>(next_ops);
		}
	}
	ops_per_sample_ = ops;

	//	3. サンプル
	const nox::uint32 sample_count = options_.smoke ? 1u : options_.sample_count;
	const double divisor = static_cast<double>(ops) * static_cast<double>(items_per_op_);
	samples_ns_.clear();
	samples_ns_.reserve(sample_count);
	cycles_per_op_.clear();
	cycles_per_op_.reserve(sample_count);
	for (nox::uint32 sample = 0u; sample < sample_count; ++sample)
	{
		const nox::uint64 cycles_begin = nox::bench::detail::ReadThreadCycles();
		const Clock::time_point begin = Clock::now();
		nox::bench::detail::InvokeBody(body, ops);
		const Clock::time_point end = Clock::now();
		const nox::uint64 cycles_end = nox::bench::detail::ReadThreadCycles();

		samples_ns_.push_back(ElapsedNs(begin, end) / divisor);
		cycles_per_op_.push_back(static_cast<double>(cycles_end - cycles_begin) / divisor);
	}

	//	4. 確保計数 (全スレッド合計。ワーカーの確保も含む)
	const nox::uint64 alloc_ops = options_.smoke ? 1u : options_.alloc_ops;
	const nox::memory::AllocationCounters counters0 = nox::memory::GetAllocationCounters();
	nox::bench::detail::InvokeBody(body, alloc_ops);
	const nox::memory::AllocationCounters counters1 = nox::memory::GetAllocationCounters();
	nox::bench::detail::InvokeBody(body, alloc_ops);
	const nox::memory::AllocationCounters counters2 = nox::memory::GetAllocationCounters();

	const double alloc_divisor = static_cast<double>(alloc_ops) * static_cast<double>(items_per_op_);
	allocs_per_op_ = static_cast<double>(counters1.allocate_count - counters0.allocate_count) / alloc_divisor;
	alloc_bytes_per_op_ = static_cast<double>(counters1.allocate_bytes - counters0.allocate_bytes) / alloc_divisor;
	frees_per_op_ = static_cast<double>(counters1.deallocate_count - counters0.deallocate_count) / alloc_divisor;
	alloc_stable_ =
		((counters1.allocate_count - counters0.allocate_count) == (counters2.allocate_count - counters1.allocate_count)) &&
		((counters1.allocate_bytes - counters0.allocate_bytes) == (counters2.allocate_bytes - counters1.allocate_bytes));
}
