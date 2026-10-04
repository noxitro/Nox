//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	thread_win64.cpp
///	@brief	thread_win64
#include	"pch.h"
#include	"../thread.h"

#if NOX_WIN64
#include	<process.h>
#include	"../win64_api.h"
#include	"../mutex.h"
#include	"../scoped_lock.h"
#include	"../assertion.h"
#include	"../algorithm.h"
#include	"../convert_string.h"
#include	"../log_trace.h"
#include	"../log_id.h"
#include	<utility>

namespace nox::detail
{
	namespace
	{
		/// @brief ネイティブハンドルテーブル
		static inline constinit std::array<::HANDLE, nox::MAX_THREAD_ID> g_native_handle_table = { nullptr };

		/// @brief スレッド管理ID割り当て用
		static inline constinit nox::Mutex g_mutex;
	}
}

struct nox::detail::ThreadDetail
{
	/// @brief スレッドに登録するコールバック関数
	/// @param argPtr ThreadHandleWin64のアドレス 
	/// @return エラーコード
	static inline nox::uint32 CALLBACK ThreadProc(void* argPtr)
	{
		static_cast<nox::Thread*>(argPtr)->Run();
		return 0;
	}
};

void	nox::Thread::Dispatch( std::move_only_function<void()> func)
{
	//	関数をセット
	thread_func_ = std::move(func);
	NOX_ASSERT(thread_func_ != nullptr, u"スレッドコールバックがnullです");
	
	native_thread_handle_ = reinterpret_cast<::HANDLE>(::_beginthreadex(
		nullptr,	//	
		stack_size_,	//	0の場合、標準のスタックサイズを使用する
		&nox::detail::ThreadDetail::ThreadProc,	//	thread関数
		this,		//	thread関数への引数
		0,			//	作成オプション
		&native_thread_id_
	));

	if (native_thread_handle_ == nullptr)
	{
		//	thread生成失敗
		return;
	}
}

void	nox::Thread::Wait()
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

nox::ThreadInfo nox::Thread::GetThreadInfo()
{
	::NT_TIB* const tib = reinterpret_cast<::NT_TIB*>(::NtCurrentTeb());
	if (tib == nullptr)
	{
		return ThreadInfo();
	}

	return ThreadInfo{
		.stackBase = static_cast<nox::uint32*>(tib->StackBase),
		.stackLimit = static_cast<nox::uint32*>(tib->StackLimit),
	};
}

void	nox::Thread::AssignThreadId()
{
	//	割り当て済みか
	if (current_thread_id_ >= 0)
	{
		return;
	}

	//	ロックする
	const nox::ScopedLock guard(nox::detail::g_mutex);

	decltype(current_thread_id_) threadId = 0;

	//	空きのハンドルを探す
	for (nox::int8 i = 0; i < static_cast<nox::int8>(nox::detail::g_native_handle_table.size()); ++i)
	{
		//	空
		if (nox::detail::g_native_handle_table[i] == nullptr)
		{
			threadId = i;
			break;
		}

		//	スレッドが終了しているか
		::DWORD exitCode = 0;
		if (::GetExitCodeThread(nox::detail::g_native_handle_table[i], &exitCode) == FALSE)
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
		&nox::detail::g_native_handle_table[threadId],
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

void nox::Thread::AssignThisNativeThreadId()
{
	local_native_thread_id_ = ::GetCurrentThreadId();
	NOX_ASSERT(local_native_thread_id_ != 0, u"ネイティブスレッドIDの取得に失敗しました");
}

void nox::Thread::SetCurrentThreadName(nox::not_null<nox::char16*> name)
{
	const ::HRESULT result = ::SetThreadDescription(
		::GetCurrentThread(),
		reinterpret_cast<const nox::wchar16*>(name.get())
	);

	if (FAILED(result))
	{
		NOX_ERROR_LINE(nox::log_id::OS, u8"ネイティブthread名の設定に失敗");
	}
}

void nox::Thread::SetCurrentThreadPriority(nox::ThreadPriority priority)
{
	static constexpr std::array<nox::int32, nox::util::ToUnderlying(nox::ThreadPriority::_Max)> table =
	{
		THREAD_PRIORITY_IDLE,
		THREAD_PRIORITY_LOWEST,
		THREAD_PRIORITY_BELOW_NORMAL,
		THREAD_PRIORITY_NORMAL,
		THREAD_PRIORITY_ABOVE_NORMAL,
		THREAD_PRIORITY_HIGHEST,
		THREAD_PRIORITY_TIME_CRITICAL
	};

	::SetThreadPriority(::GetCurrentThread(), table[(nox::util::ToUnderlying(priority))]);
}

#endif // _WIN64
