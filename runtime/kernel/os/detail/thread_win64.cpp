//	Copyright (C) 2024 NOX ENGINE All Rights Rserved.

///	@file	thread_win64.cpp
///	@brief	thread_win64
#include	"stdafx.h"
#include	"thread_win64.h"

#if NOX_WIN64
#include	<process.h>

#include	"mutex_win64.h"
#include	"../../assertion.h"
#include	"../../algorithm.h"
#include	"../../convert_string.h"
#include	"../../log_trace.h"
#include	"../../log_id.h"

namespace
{
	/// @brief ネイティブハンドルテーブル
	static inline constinit std::array<::HANDLE, nox::os::MAX_THREAD_ID> g_native_handle_table = { nullptr };

	/// @brief スレッド管理ID割り当て用
	static inline nox::os::detail::MutexWin64 g_mutex;
}

nox::os::detail::ThreadWin64::~ThreadWin64()noexcept
{
	Wait();
}

void	nox::os::detail::ThreadWin64::Dispatch( std::function<void()> func)
{
	//	関数をセット
	thread_func_ = func;
	NOX_ASSERT(thread_func_ != nullptr, u"スレッドコールバックがnullです");

	//	終了していなければ待つ
	Wait();
	
	native_thread_handle_ = reinterpret_cast<::HANDLE>(::_beginthreadex(
		nullptr,	//	
		stack_size_,	//	0の場合、標準のスタックサイズを使用する
		&ThreadWin64::ThreadProc,	//	thread関数
		this,		//	thread関数への引数
		0,			//	作成オプション
		&native_thread_id_
	));

	if (native_thread_handle_ == nullptr)
	{
		//	thread生成失敗
		return;
	}

	//	優先度をセット
	static constexpr std::array<int32, nox::util::ToUnderlying(ThreadPriority::_Max)> THREAD_PRIORITY_TABLE =
	{
		THREAD_PRIORITY_IDLE,
		THREAD_PRIORITY_LOWEST,
		THREAD_PRIORITY_BELOW_NORMAL,
		THREAD_PRIORITY_NORMAL,
		THREAD_PRIORITY_ABOVE_NORMAL,
		THREAD_PRIORITY_HIGHEST,
		THREAD_PRIORITY_TIME_CRITICAL
	};

	::SetThreadPriority(
		native_thread_handle_,
		THREAD_PRIORITY_TABLE.at(nox::util::ToUnderlying(thread_priority_))
	);

	//	ネイティブスレッド名をセット
	const ::HRESULT threadDescriptionResult = ::SetThreadDescription(
		native_thread_handle_,
		reinterpret_cast<const wchar16*>(thread_name_.data())
	);

	NOX_ASSERT(FAILED(threadDescriptionResult) == false, u"ネイティブthread名の設定に失敗");

	thread_state_ = ThreadState::Work;
}

void	nox::os::detail::ThreadWin64::Wait()
{
	if (native_thread_handle_ == nullptr)
	{
	//	NOX_ERROR_LINE(nox::log_id::OS, u"native_thread_handle_ is null");
		return;
	}

	::WaitForSingleObject(native_thread_handle_, INFINITE);
	::CloseHandle(native_thread_handle_);
	native_thread_handle_ = nullptr;

	thread_state_ = ThreadState::Wait;
}

nox::os::ThreadInfo nox::os::detail::ThreadWin64::GetThreadInfo()
{
	::NT_TIB* const tib = reinterpret_cast<::NT_TIB*>(::NtCurrentTeb());
	if (tib == nullptr)
	{
		return ThreadInfo();
	}

	return ThreadInfo{
		.stackBase = static_cast<uint32*>(tib->StackBase),
		.stackLimit = static_cast<uint32*>(tib->StackLimit),
	};
}

void	nox::os::detail::ThreadWin64::AssignThreadId()
{
	//	割り当て済みか
	if (current_thread_id_ >= 0)
	{
		return;
	}

	//	ロックする
	const ScopedLock guard(g_mutex);

	decltype(current_thread_id_) threadId = 0;

	//	空きのハンドルを探す
	for (int8 i = 0; i < static_cast<int8>(g_native_handle_table.size()); ++i)
	{
		//	空
		if (g_native_handle_table[i] == nullptr)
		{
			threadId = i;
			break;
		}

		//	スレッドが終了しているか
		::DWORD exitCode = 0;
		if (::GetExitCodeThread(g_native_handle_table[i], &exitCode) == FALSE)
		{
			continue;
		}

		switch (exitCode)
		{
			//	稼働中
		case STILL_ACTIVE:
			continue;
		}

		threadId = i;
		break;
	}

	//	空きスレッドが見つからなかった
	NOX_ASSERT(threadId >= 0, u"空きスレッドが見つかりませんでした");

	if (::DuplicateHandle(
		::GetCurrentProcess(),
		::GetCurrentThread(),
		::GetCurrentProcess(),
		&g_native_handle_table[threadId],
		0,
		FALSE,
		DUPLICATE_SAME_ACCESS
	) == FALSE)
	{
		NOX_ASSERT(false, u"スレッドの複製に失敗しました");
		return;
	}

	current_thread_id_ = threadId;
	
	++thread_counter_;
}

inline nox::uint32 CALLBACK nox::os::detail::ThreadWin64::ThreadProc(void* argPtr)
{
	ThreadWin64* const this_ptr = static_cast<ThreadWin64*>(argPtr);
	NOX_ASSERT(this_ptr != nullptr, u"ThreadWin64::ThreadProc argPtr is null");

	current_thread_ = this_ptr;

	//	通常関数実行
	this_ptr->thread_func_();

	//	スレッドの終了関数実行
	auto&& terminateFunc = terminate_func_table_.at(GetThreadId());
	if (terminateFunc != nullptr)
	{
		terminateFunc();
	}

	return 0;
}

void nox::os::detail::ThreadWin64::AssignThisNativeThreadId()
{
	local_native_thread_id_ = ::GetCurrentThreadId();
	NOX_ASSERT(local_native_thread_id_ != 0, u"ネイティブスレッドIDの取得に失敗しました");
}

#endif // _WIN64
