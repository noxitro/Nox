//	Copyright (C) 2024 NOX ENGINE All Rights Rserved.

///	@file	thread_base.h
///	@brief	thread_base
#pragma once
#include	"../os_utility.h"

#include	"../../advanced_type.h"
#include	"../../delegate.h"

namespace nox::os
{
	/**
	 * @brief 最大ThreadID
	*/
	constexpr uint8 MAX_THREAD_ID = 64;
	static_assert(MAX_THREAD_ID < std::numeric_limits<uint8>::max());

	///**
	// * @brief スレッドプール
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

	/**
	 * @brief スレッド情報
	*/
	struct ThreadInfo
	{
		uint32* stackBase;
		uint32* stackLimit;
	};

	/**
	 * @brief スレッド優先度
	*/
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

	/**
	 * @brief スレッド基底(ダミー)
	 * Empty Base
	*/
	class ThreadInterface
	{
	protected:

	};

	/**
	 * @brief スレッド基底
	 * Empty Base
	*/
	class ThreadBase : public ThreadInterface
	{
	public:
		inline constexpr ThreadBase()noexcept :
			thread_id_(-1),
			thread_state_(ThreadState::Wait),
			thread_priority_(ThreadPriority::Normal),
			stack_size_(0)
		{
			//	ThreadPool::Instance()->RegisterHandle(T::GetThreadId(), this);
		}

		inline ~ThreadBase() {
			//	ThreadPool::Instance()->UnregisterHandle(T::GetThreadId());
		}

		/**
			 * @brief CPUスレッド数を取得する
			 * @return
			*/
		static inline int8 GetHardwareConcurrency()noexcept {
			return os::GetHardwareConcurrency();
		}

		inline constexpr explicit ThreadBase(const ThreadBase&)noexcept = delete;
		inline constexpr explicit ThreadBase(const ThreadBase&&)noexcept = delete;

		static void	SetThreadName(std::u16string_view name);
		static inline std::u16string_view	GetThreadName()noexcept { return thread_name_.data(); }
		inline	constexpr	ThreadState GetThreadState()const noexcept { return thread_state_; }
		inline	constexpr	void SetThreadPriority(ThreadPriority priority)noexcept { thread_priority_ = priority; }
		inline	constexpr	ThreadPriority GetThreadPriority()const noexcept { return thread_priority_; }
		inline	constexpr	void SetStackSize(const int32 stackSize)noexcept { stack_size_ = stackSize; }

	/*	template<class FuncType, class... Args> requires(std::is_invocable_v<FuncType, Args...>)
			static inline	constexpr	void SetTerminateFunc(FuncType func, const Args&... args)
		{
			terminate_func_table_.at(T::GetThreadId()) = [&func, &args...]() {std::invoke(func, args...); };
		}*/
	protected:

		/**
		 * @brief スレッドID
		*/
		static constinit inline thread_local int8 current_thread_id_ = -1;

		/**
		 * @brief 管理スレッド数
		*/
		static constinit inline int8 thread_counter_ = 0;

		/**
		 * @brief スレッド名
		*/
		static inline constinit thread_local std::array<char16, 256> thread_name_ = { 0 };

		/**
		 * @brief スレッドID
		*/
		int8 thread_id_;

		/// @brief スレッド状態
		ThreadState thread_state_;

		/**
		 * @brief 優先度
		*/
		ThreadPriority thread_priority_;

		/**
		 * @brief スタックサイズ
		*/
		int32 stack_size_;

		/**
		 * @brief スレッド実行関数
		*/
	//	Delegate<void()> thread_func_;

		/**
		 * @brief スレッド終了時の関数テーブル
		*/
	//	static inline constinit std::array<Delegate<void()>, MAX_THREAD_ID> terminate_func_table_;
	};


}