//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	window.cpp
///	@brief	window
#include	"pch.h"
#include	"window.h"

#include	<filesystem>
#if NOX_WINDOWS
#include	"win64_api.h"
#endif // NOX_WINDOWS

#include	"basic_definition.h"
#include	"assertion.h"
#include	"os.h"
#include	"mutex.h"
#include	"log_trace.h"
#include	"log_id.h"
#include	"preprocessor/util.h"
#include	"parallel_execute_checker.h"
#include	"scoped_lock.h"

namespace nox
{
	namespace
	{

		struct MessageHookEntry
		{
			void(*hook)(const nox::WindowMessage& message, void* user_data);
			void* user_data;
		};

		constexpr nox::uint8 kMaxWindowMessageHooks = 8u;
		constinit std::array<nox::MessageHookEntry, kMaxWindowMessageHooks> window_message_hooks_{};
		constinit nox::uint8 window_message_hook_count_ = 0u;

		nox::Mutex window_message_hooks_mutex_{};

#if !NOX_MASTER
		nox::util::RWParallelExecuteChecker window_message_hooks_checker_{};
#endif // !NOX_MASTER
	}
}

nox::Window::Window()noexcept:
	is_visible_(false),
	window_handle_(nullptr),
	instance_handle_(nullptr),
	callback_(nullptr)
{

}

nox::Window::~Window()
{
	Dispose();
}

std::array<nox::char16, nox::Window::k_max_title_length> nox::Window::GetWindowTitle()const noexcept
{
	std::array<nox::char16, k_max_title_length> title_buffer{0};
	this->GetWindowTitle(std::span<nox::char16>(title_buffer.data(), title_buffer.size()));
	return title_buffer;
}

bool nox::Window::RegisterMessageHook(void(&func)(const nox::WindowMessage& message, void* user_data), void* user_data)
{
	//	スレッドセーフ対応
	//	kMaxWindowMessageHooks を超えないようにする
	NOX_LOCAL_SCOPE(nox::ScopedLock(nox::window_message_hooks_mutex_));
#if !NOX_MASTER
	NOX_LOCAL_SCOPE(nox::util::WriteParallelExecuteCheckScope(nox::window_message_hooks_checker_, nox::util::detail::ParallelExecuteCheckOption::StackTrace));
#endif // !NOX_MASTER

	if (nox::window_message_hook_count_ >= nox::kMaxWindowMessageHooks)
	{
		NOX_ERROR_LINE(nox::log_id::OS, u8"ウィンドウメッセージのフックが上限に達しました");
		return false;
	}
	
	nox::window_message_hooks_[nox::window_message_hook_count_] = nox::MessageHookEntry{ func, user_data };
	++nox::window_message_hook_count_;
	return true;
}

bool nox::Window::UnregisterMessageHook(void(&func)(const nox::WindowMessage& message, void* user_data), void* user_data)
{
	NOX_LOCAL_SCOPE(nox::ScopedLock(nox::window_message_hooks_mutex_));
#if !NOX_MASTER
	NOX_LOCAL_SCOPE(nox::util::WriteParallelExecuteCheckScope(nox::window_message_hooks_checker_, nox::util::detail::ParallelExecuteCheckOption::StackTrace));
#endif // !NOX_MASTER

	//	見つけた位置から後続を詰める
	for (nox::uint32 hook_index = 0u; hook_index < nox::window_message_hook_count_; ++hook_index)
	{
		const nox::MessageHookEntry& entry = nox::window_message_hooks_[hook_index];
		if (entry.hook == func && entry.user_data == user_data)
		{
			for (nox::uint32 move_index = hook_index; move_index + 1u < nox::window_message_hook_count_; ++move_index)
			{
				nox::window_message_hooks_[move_index] = nox::window_message_hooks_[move_index + 1u];
			}
			--nox::window_message_hook_count_;
			return true;
		}
	}

	NOX_ERROR_LINE(nox::log_id::OS, u8"ウィンドウメッセージのフックが見つかりませんでした");
	return false;
}

/// @brief 汎用購読者にウィンドウメッセージを通知する
		/// @param message 
void nox::Window::DispatchWindowMessageHooks(const nox::WindowMessage& message)noexcept
{
#if !NOX_MASTER
	NOX_LOCAL_SCOPE(nox::util::ReadParallelExecuteCheckScope(nox::window_message_hooks_checker_, nox::util::detail::ParallelExecuteCheckOption::StackTrace));
#endif // !NOX_MASTER

	for (nox::uint8 hook_index = 0u; hook_index < nox::window_message_hook_count_; ++hook_index)
	{
		const nox::MessageHookEntry& entry = nox::window_message_hooks_[hook_index];
		if (entry.hook != nullptr)
		{
			entry.hook(message, entry.user_data);
		}
	}
}