//	Copyright (C) 2024 NOX ENGINE All Rights Rserved.

///	@file	parallel_execute_checker.h
///	@brief	parallel_execute_checker
#pragma once
#include	"os/atomic.h"
#include	"log_trace.h"

namespace nox::util
{
#if !NOX_MASTER
	/// @brief 並列実行チェッカー
	class ParallelExecuteChecker
	{
	public:
		inline consteval ParallelExecuteChecker()noexcept :ref_counter_(0) {}
		inline ~ParallelExecuteChecker()noexcept {}

		inline	consteval ParallelExecuteChecker(const ParallelExecuteChecker&)noexcept = delete;
		inline	consteval	ParallelExecuteChecker(ParallelExecuteChecker&&)noexcept = delete;

		inline consteval	ParallelExecuteChecker& operator =(const ParallelExecuteChecker&)noexcept = delete;

		inline	void Enter()
		{
			if (nox::os::atomic::Increment(ref_counter_) > 1)
			{
				NOX_ERROR_LINE(U"ParallelExecuteCheck failed.");
			}
		}

		inline	void Exit()
		{
			nox::os::atomic::Decrement(ref_counter_);
		}
	private:
		nox::int16 ref_counter_;
	};

	/// @brief 並列実行チェック
	class ParallelExecuteCheck
	{
	public:
		inline ParallelExecuteCheck(ParallelExecuteChecker& checker, const std::source_location location = std::source_location::current()) :
			checker_(checker)
		{
			checker_.Enter();
		}

		inline ~ParallelExecuteCheck()
		{
			checker_.Exit();
		}

		inline constexpr ParallelExecuteCheck(const ParallelExecuteCheck&)noexcept = delete;
		inline constexpr ParallelExecuteCheck(ParallelExecuteCheck&&)noexcept = delete;

		inline constexpr ParallelExecuteCheck& operator =(const ParallelExecuteCheck&)noexcept = delete;
	private:
		ParallelExecuteChecker& checker_;
	};

#endif // !NOX_MASTER
}