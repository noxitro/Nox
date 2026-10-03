//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	thread.h
///	@brief	thread
#pragma once
#include	"basic_definition.h"
#include	"os_definition.h"
#include	"advanced_type.h"
#include	"advanced_definition.h"

namespace nox
{
#if NOX_WIN64
	using NativeThreadHandle = void*;
#endif // NOX_WIN64
}

namespace nox
{
	/// @brief 最大ThreadID
	constexpr nox::int8 MAX_THREAD_ID = 64;
	static_assert(MAX_THREAD_ID < std::numeric_limits<nox::int8>::max());

	/// @brief スレッドプール
	//*/
	//class ThreadPool
	//{
	//public:
	//	static inline ThreadPool* Instance()noexcept
	//	{
	//		static ThreadPool instance;
	//		return &instance;
	//	}

	//	void	RegisterHandle(uint8 threadId, class ThreadInterface* handlePtr);
	//	void	UnregisterHandle(uint8 threadId);

	//	void	Finalize();

	//	inline class ThreadInterface* GetThread(const uint8 threadId)const noexcept { return mThreadHandlePtrTbl.at(threadId); }
	//private:
	//	ThreadPool();
	//	~ThreadPool() = default;
	//private:
	//	/**
	//	 * @brief スレッドハンドル配列
	//	*/
	//	std::array<class ThreadInterface*, MAX_THREAD_ID> mThreadHandlePtrTbl;
	//	//		std::array<Thread*, MAX_THREAD_ID> mThreadPtrTbl;
	//};

	/// @brief スレッド情報
	struct ThreadInfo
	{
		nox::uint32* stackBase;
		nox::uint32* stackLimit;
	};

	/// @brief スレッド優先度
	enum class ThreadPriority : uint8
	{
		Idle,
		Lowest,
		BelowNormal,
		Normal,
		AboveNormal,
		Highest,
		TimeCritical,

		_Max
	};

	namespace detail
	{
		/// @brief 内部実装へのfriend宣言
		struct ThreadDetail;
	}

	/// @brief スレッド
	class Thread
	{
		friend nox::detail::ThreadDetail;
	public:
		inline Thread()noexcept :
			thread_id_(-1),
			thread_state_(nox::ThreadState::Wait),
			thread_priority_(nox::ThreadPriority::Normal),
			stack_size_(0),
			native_thread_handle_(nullptr),
			native_thread_id_(0)
		{
			//	ThreadPool::Instance()->RegisterHandle(T::GetThreadId(), this);
		}

		inline ~Thread() {
			Wait();
		}

		/// @brief CPUスレッド数を取得する
		//static inline int8 GetHardwareConcurrency()noexcept {
//			return nox::os::GetHardwareConcurrency();
		//}

		inline constexpr explicit Thread(const Thread&)noexcept = delete;
		inline constexpr explicit Thread(const Thread&&)noexcept = delete;

		void	SetThreadName(std::u16string_view name);
		inline std::u16string_view	GetThreadName()noexcept { return thread_name_.data(); }
		inline	constexpr	ThreadState GetThreadState()const noexcept { return thread_state_; }
		inline	constexpr	void SetThreadPriority(ThreadPriority priority)noexcept { thread_priority_ = priority; }
		inline	constexpr	ThreadPriority GetThreadPriority()const noexcept { return thread_priority_; }
		inline	constexpr	void SetStackSize(const int32 stackSize)noexcept { stack_size_ = stackSize; }

		/*	template<class FuncType, class... Args> requires(std::is_invocable_v<FuncType, Args...>)
				static inline	constexpr	void SetTerminateFunc(FuncType func, const Args&... args)
			{
				terminate_func_table_.at(T::GetThreadId()) = [&func, &args...]() {std::invoke(func, args...); };
			}*/
		static void	Sleep(nox::uint32 milliseccond);

		/// @brief スレッド管理IDを取得
	
		static inline nox::int8	GetThreadId() {
			//	まだアサインされていない
			if (current_thread_id_ < 0)
			{
				//	割り当て
				AssignThreadId();
			}

			return current_thread_id_;
		}

		/// @brief スレッドの実行
	/// @param func 
		void Dispatch(std::move_only_function<void()> func);

		/// @brief 停止
		void	Wait();

		/// @brief スレッド情報を取得
		/// @return	
		static nox::ThreadInfo GetThreadInfo();

		inline constexpr nox::NativeThreadHandle GetNativeHandle()const noexcept { return native_thread_handle_; }
		inline constexpr nox::uint32 GetNativeThreadId()const noexcept { return native_thread_id_; }
		static inline constexpr Thread& GetCurrentThread()noexcept { return nox::util::Deref(current_thread_); }

		static inline nox::uint32 GetThisThreadNativeThreadId()
		{
			if (local_native_thread_id_ == 0)
			{
				AssignThisNativeThreadId();
			}
			return local_native_thread_id_;
		}

	private:
		/// @brief スレッド管理IDを割り当てる
		static void	AssignThreadId();

		static void AssignThisNativeThreadId();

		void Run();

		/// @brief Nativeスレッドの名前を設定する
		/// @param name null終端されている文字列
		static void SetCurrentThreadName(nox::not_null<nox::char16*> name);
		static void SetCurrentThreadPriority(nox::ThreadPriority priority);
	protected:
		/// @brief スレッドID
		static constinit inline thread_local nox::int8 current_thread_id_ = -1;

		/// @brief 管理スレッド数
		static constinit inline nox::int8 thread_counter_ = 0;

		/// @brief スレッド名
		std::array<nox::char16, 256> thread_name_ = { 0 };

		/// @brief スレッドID
		nox::int8 thread_id_;

		/// @brief スレッド状態
		nox::ThreadState thread_state_;

		/// @brief 優先度
		nox::ThreadPriority thread_priority_;

		/// @brief スタックサイズ
		nox::int32 stack_size_;

		/// @brief スレッド実行関数
		std::move_only_function<void()> thread_func_;

		/// @brief スレッド終了時の関数
		std::function<void()> terminate_func_;

		/// @brief ネイティブスレッドハンドル
		nox::NativeThreadHandle native_thread_handle_;

		/// @brief ネイティブスレッドID
		nox::uint32 native_thread_id_;

		/// @brief 現在のスレッド
		static inline thread_local constinit Thread* current_thread_ = nullptr;

		static inline thread_local constinit nox::uint32 local_native_thread_id_ = 0;
	};
}
