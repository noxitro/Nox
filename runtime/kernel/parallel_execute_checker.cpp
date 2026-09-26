//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	parallel_execute_checker.cpp
///	@brief	parallel_execute_checker
#include	"pch.h"
#include	"parallel_execute_checker.h"

#if !NOX_MASTER

#include	"os/atomic.h"
#include	"os/thread.h"
#include	"assertion.h"
#include	"log_id.h"
#include	"stack_trace.h"

namespace
{
	//	RWParallelExecuteChecker の状態ワードのレイアウト。
	//	bit  0..23 : Reader数 / bit 24..31 : Writer再入深度 / bit 32..63 : Writer保持スレッドID
	constexpr nox::uint32 k_rw_reader_shift = 0u;
	constexpr nox::uint64 k_rw_reader_mask = 0x0000000000ffffffull;
	constexpr nox::uint32 k_rw_writer_shift = 24u;
	constexpr nox::uint64 k_rw_writer_mask = 0x00000000ff000000ull;
	constexpr nox::uint32 k_rw_owner_shift = 32u;
	constexpr nox::uint64 k_rw_owner_mask = 0xffffffff00000000ull;

	constexpr nox::uint64 k_rw_reader_one = 1ull << k_rw_reader_shift;
	constexpr nox::uint64 k_rw_writer_one = 1ull << k_rw_writer_shift;

	[[nodiscard]] constexpr nox::uint32 get_rw_reader_count(const nox::uint64 state)noexcept
	{
		return static_cast<nox::uint32>((state & k_rw_reader_mask) >> k_rw_reader_shift);
	}

	[[nodiscard]] constexpr nox::uint32 get_rw_writer_depth(const nox::uint64 state)noexcept
	{
		return static_cast<nox::uint32>((state & k_rw_writer_mask) >> k_rw_writer_shift);
	}

	[[nodiscard]] constexpr nox::uint32 get_rw_owner_thread_id(const nox::uint64 state)noexcept
	{
		return static_cast<nox::uint32>((state & k_rw_owner_mask) >> k_rw_owner_shift);
	}

	[[nodiscard]] constexpr nox::uint64 make_rw_owner_bits(const nox::uint32 thread_id)noexcept
	{
		return static_cast<nox::uint64>(thread_id) << k_rw_owner_shift;
	}
}

nox::util::ParallelExecuteCheckScope::ParallelExecuteCheckScope(nox::util::ParallelExecuteChecker& checker, nox::util::detail::ParallelExecuteCheckOption option, const std::source_location location) :
	checker_(checker)
{
	checker_.Enter(option, location);
}

nox::util::ParallelExecuteCheckScope::~ParallelExecuteCheckScope()
{
	checker_.Exit();
}

nox::util::ReadParallelExecuteCheckScope::ReadParallelExecuteCheckScope(nox::util::RWParallelExecuteChecker& checker, nox::util::detail::ParallelExecuteCheckOption option, const std::source_location location) :
	checker_(checker)
{
	checker_.EnterRead(option, location);
}

nox::util::ReadParallelExecuteCheckScope::~ReadParallelExecuteCheckScope()
{
	checker_.ExitRead();
}

nox::util::WriteParallelExecuteCheckScope::WriteParallelExecuteCheckScope(nox::util::RWParallelExecuteChecker& checker, nox::util::detail::ParallelExecuteCheckOption option, const std::source_location location) :
	checker_(checker)
{
	checker_.EnterWrite(option, location);
}

nox::util::WriteParallelExecuteCheckScope::~WriteParallelExecuteCheckScope()
{
	checker_.ExitWrite();
}

void nox::util::ParallelExecuteChecker::Enter(nox::util::detail::ParallelExecuteCheckOption option, const std::source_location& location)
{
	if (nox::os::atomic::Increment(ref_counter_) > 1)
	{
		if (option == nox::util::detail::ParallelExecuteCheckOption::SourceLocation)
		{
			NOX_ERROR_LINE(nox::log_id::Kernel, u"並列実行を検知しました {}:{}", location.file_name(), location.line());
		}
		else
		{
			nox::stack_walker::StackWalkerSlim walker;
			walker.Collect();
			NOX_ERROR_LINE(nox::log_id::Kernel, u"並列実行を検知しました");
			walker.Trace();
		}
	}
}

void nox::util::ParallelExecuteChecker::Exit()
{
	nox::os::atomic::Decrement(ref_counter_);
}

void nox::util::RWParallelExecuteChecker::ReportViolation(
	const std::u8string_view access_kind,
	const nox::uint64 observed_state,
	const nox::util::detail::ParallelExecuteCheckOption option,
	const std::source_location& location)const
{
	const std::string_view label = name_.empty() ? std::string_view("(unnamed)") : name_;

	if (option == nox::util::detail::ParallelExecuteCheckOption::StackTrace)
	{
		nox::stack_walker::StackWalkerSlim walker;
		walker.Collect();
		walker.Trace();
	}

	NOX_ASSERT(false,
		u8"並列実行を検知しました [{0}] {1} reader={2} writer={3} owner_thread={4} at {5}:{6}",
		label,
		access_kind,
		get_rw_reader_count(observed_state),
		get_rw_writer_depth(observed_state),
		get_rw_owner_thread_id(observed_state),
		location.file_name(),
		static_cast<nox::uint32>(location.line()));
}

void nox::util::RWParallelExecuteChecker::EnterRead(const nox::util::detail::ParallelExecuteCheckOption option, const std::source_location& location)
{
	const nox::uint32 this_thread_id = nox::os::Thread::GetThisThreadNativeThreadId();

	nox::uint64 state = state_.load(std::memory_order_acquire);
	for (;;)
	{
		const nox::uint32 writer_depth = get_rw_writer_depth(state);
		//	Writerが自分自身なら再入。他スレッドのWriterと重なったら宣言が破れている。
		const bool violated = (writer_depth != 0u) && (get_rw_owner_thread_id(state) != this_thread_id);

		//	Reader数だけ増やす。Writerのフィールドには触れない。
		const nox::uint64 next_state = state + k_rw_reader_one;
		if (state_.compare_exchange_weak(state, next_state, std::memory_order_acq_rel, std::memory_order_acquire) == false)
		{
			continue;
		}

		if (violated)
		{
			ReportViolation(u8"read", state, option, location);
		}
		return;
	}
}

void nox::util::RWParallelExecuteChecker::ExitRead()
{
	state_.fetch_sub(k_rw_reader_one, std::memory_order_release);
}

void nox::util::RWParallelExecuteChecker::EnterWrite(const nox::util::detail::ParallelExecuteCheckOption option, const std::source_location& location)
{
	const nox::uint32 this_thread_id = nox::os::Thread::GetThisThreadNativeThreadId();

	nox::uint64 state = state_.load(std::memory_order_acquire);
	for (;;)
	{
		const nox::uint32 writer_depth = get_rw_writer_depth(state);
		const bool owned_by_this_thread = (writer_depth != 0u) && (get_rw_owner_thread_id(state) == this_thread_id);

		//	自分がWriterを保持しているなら再入。そうでなければReaderもWriterも1つでも居たら違反。
		const bool violated = owned_by_this_thread
			? false
			: ((writer_depth != 0u) || (get_rw_reader_count(state) != 0u));

		//	違反していても深度は必ず積む。ExitWriteと対で釣り合わせるため。
		const nox::uint64 next_state = owned_by_this_thread
			? (state + k_rw_writer_one)
			: ((state & ~k_rw_owner_mask & ~k_rw_writer_mask) | k_rw_writer_one | make_rw_owner_bits(this_thread_id));

		if (state_.compare_exchange_weak(state, next_state, std::memory_order_acq_rel, std::memory_order_acquire) == false)
		{
			continue;
		}

		if (violated)
		{
			ReportViolation(u8"write", state, option, location);
		}
		return;
	}
}

void nox::util::RWParallelExecuteChecker::ExitWrite()
{
	nox::uint64 state = state_.load(std::memory_order_acquire);
	for (;;)
	{
		const nox::uint32 writer_depth = get_rw_writer_depth(state);
		if (writer_depth == 0u)
		{
			//	対になるEnterWriteが無い。呼び出し側のバグだが、状態は壊さずに抜ける。
			NOX_ASSERT(false, u8"ExitWriteがEnterWriteと対応していません");
			return;
		}

		//	最後の1つを抜けるときだけ所有スレッドを消す。
		const nox::uint64 next_state = (writer_depth == 1u)
			? (state & ~k_rw_owner_mask & ~k_rw_writer_mask)
			: (state - k_rw_writer_one);

		if (state_.compare_exchange_weak(state, next_state, std::memory_order_acq_rel, std::memory_order_acquire))
		{
			return;
		}
	}
}

#endif // !NOX_MASTER