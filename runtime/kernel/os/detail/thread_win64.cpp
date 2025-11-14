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

namespace
{
	/**
	 * @brief ネイティブハンドルテーブル
	*/
	static inline constinit std::array<::HANDLE, nox::os::MAX_THREAD_ID> gNativeHandleTable = { nullptr };

	/**
	 * @brief スレッド管理ID割り当て用
	*/
	static inline nox::os::detail::MutexWin64 mMutex;
}

nox::os::detail::ThreadWin64::~ThreadWin64()noexcept
{
	Wait();
}
#if false
void	nox::os::detail::ThreadWin64::Dispatch(const Delegate<void()>& func)
{
	//	関数をセット
	thread_func_ = func;
	NOX_ASSERT(thread_func_.IsEmpty() == false, U"スレッドコールバックがnullです");

	//	終了していなければ待つ
	Wait();

	native_thread_handle_ = reinterpret_cast<::HANDLE>(::_beginthreadex(
		nullptr,	//	
		stack_size_,	//	標準のスタックサイズを使用する
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

	thread_state_ = ThreadState::Work;
}
#endif

void	nox::os::detail::ThreadWin64::Wait()
{
	if (native_thread_handle_ == nullptr)
	{
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
	const ScopedLock guard(mMutex);

	decltype(current_thread_id_) threadId = 0;

	//	空きのハンドルを探す
	for (int8 i = 0; i < static_cast<int8>(gNativeHandleTable.size()); ++i)
	{
		//	空
		if (gNativeHandleTable[i] == nullptr)
		{
			threadId = i;
			break;
		}

		//	スレッドが終了しているか
		::DWORD exitCode = 0;
		if (::GetExitCodeThread(gNativeHandleTable[i], &exitCode) == FALSE)
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
	NOX_ASSERT(threadId >= 0, U"空きスレッドが見つかりませんでした");

	if (::DuplicateHandle(
		GetCurrentProcess(),
		GetCurrentThread(),
		GetCurrentProcess(),
		&gNativeHandleTable[threadId],
		0,
		FALSE,
		DUPLICATE_SAME_ACCESS
	) == FALSE)
	{
		NOX_ASSERT(false, U"スレッドの複製に失敗しました");
		return;
	}

	current_thread_id_ = threadId;
	++thread_counter_;
}

inline nox::uint32 CALLBACK nox::os::detail::ThreadWin64::ThreadProc(void* argPtr)
{
	ThreadWin64* const thisPtr = static_cast<ThreadWin64*>(argPtr);
	if (thisPtr == nullptr)
	{
		return 0;
	}
#if false
	if (thisPtr->thread_func_.IsEmpty() == false)
	{
		//	通常関数実行
		thisPtr->thread_func_.Invoke();
	}

	//	スレッドの終了関数実行
	const nox::Delegate<void()>& terminateFunc = terminate_func_table_.at(GetThreadId());
	if (terminateFunc.IsEmpty() == false)
	{
	//	terminateFunc.Invoke();
	}
#endif
	return 0;
}

#endif // _WIN64
