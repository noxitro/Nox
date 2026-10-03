//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	thread.cpp
///	@brief	thread
#include	"pch.h"
#include	"thread.h"

#include	"os.h"
#include	"assertion.h"
namespace nox
{
	namespace
	{
	}
}

void	nox::Thread::SetThreadName(std::u16string_view name)
{
	NOX_ASSERT(native_thread_handle_ == 0, u"SetName は Dispatch の前に呼ぶ");
	thread_name_.fill(u'\0');
	std::size_t length = std::min(name.size(), thread_name_.size() - 1);
	std::ranges::copy_n(name.data(), length, thread_name_.data());
}

void	nox::Thread::Sleep(nox::uint32 milliseccond)
{
	nox::os::Sleep(milliseccond);
}

void	nox::Thread::Run()
{
	current_thread_ = this;
	thread_state_ = ThreadState::Work;
	SetCurrentThreadName(thread_name_.data());
	SetCurrentThreadPriority(thread_priority_);
	std::invoke(thread_func_);
	thread_func_ = {};

	if (terminate_func_)
	{
		std::invoke(terminate_func_);
	}
	thread_state_ = ThreadState::Terminated;
	current_thread_ = nullptr;
}