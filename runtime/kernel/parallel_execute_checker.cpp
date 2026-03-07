//	Copyright (C) 2024 NOX ENGINE All Rights Rserved.

///	@file	parallel_execute_checker.cpp
///	@brief	parallel_execute_checker
#include	"stdafx.h"
#include	"parallel_execute_checker.h"

#if !NOX_MASTER

#include	"os/atomic.h"
#include	"log_id.h"
#include	"stack_trace.h"

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
	checker_.Enter(option, location);
}

nox::util::ReadParallelExecuteCheckScope::~ReadParallelExecuteCheckScope()
{
	checker_.Exit();
}

nox::util::WriteParallelExecuteCheckScope::WriteParallelExecuteCheckScope(nox::util::RWParallelExecuteChecker& checker, nox::util::detail::ParallelExecuteCheckOption option, const std::source_location location) :
	checker_(checker)
{
	checker_.Enter(option, location);
}

nox::util::WriteParallelExecuteCheckScope::~WriteParallelExecuteCheckScope()
{
	checker_.Exit();
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

void nox::util::RWParallelExecuteChecker::Enter(nox::util::detail::ParallelExecuteCheckOption option, const std::source_location& location)
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

void nox::util::RWParallelExecuteChecker::Exit()
{
	nox::os::atomic::Decrement(ref_counter_);
}

#endif // !NOX_MASTER