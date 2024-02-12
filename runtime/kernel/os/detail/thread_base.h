//	Copyright (C) 2024 NOX ENGINE All Rights Rserved.

///	@file	thread_base.h
///	@brief	thread_base
#pragma once
#include	"../os_utility.h"

#include	<limits>
#include	<array>

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

	///**
	// * @brief スレッド基底
	// * Empty Base
	//*/
	//class ThreadBase : public ThreadInterface
	//{
	//public:
	//	inline constexpr ThreadBase()noexcept :
	//		mThreadId(-1),
	//		mThreadState(ThreadState::Wait),
	//		mThreadPriority(ThreadPriority::Normal),
	//		mStackSize(0),
	//		mThreadFunc(nullptr)
	//	{
	//		//	ThreadPool::Instance()->RegisterHandle(T::GetThreadId(), this);
	//	}

	//	inline ~ThreadBase() {
	//		//	ThreadPool::Instance()->UnregisterHandle(T::GetThreadId());
	//	}

	//	/**
	//		 * @brief CPUスレッド数を取得する
	//		 * @return
	//		*/
	//	static inline s8 GetHardwareConcurrency()noexcept {
	//		return os::GetHardwareConcurrency();
	//	}

	//	inline constexpr explicit ThreadBase(const ThreadBase&)noexcept = delete;
	//	inline constexpr explicit ThreadBase(const ThreadBase&&)noexcept = delete;

	//	static inline void	SetThreadName(const std::wstring_view name)noexcept { mThreadName = name; }
	//	static inline std::wstring_view	GetThreadName()noexcept { return mThreadName; }
	//	inline	constexpr	ThreadState GetThreadState()const noexcept { return mThreadState; }
	//	inline	constexpr	void SetThreadPriority(ThreadPriority priority)noexcept { mThreadPriority = priority; }
	//	inline	constexpr	ThreadPriority GetThreadPriority()const noexcept { return mThreadPriority; }
	//	inline	constexpr	void SetStackSize(const u32 stackSize)noexcept { mStackSize = stackSize; }

	//	template<class FuncType, class... Args> requires(std::is_invocable_v<FuncType, Args...>)
	//		static inline	constexpr	void SetTerminateFunc(FuncType func, const Args&... args)
	//	{
	//		mTerminateFuncTable.at(T::GetThreadId()) = [&func, &args...]() {std::invoke(func, args...); };
	//	}
	//protected:

	//	/**
	//	 * @brief スレッドID
	//	*/
	//	static inline thread_local s8 mCurrentThreadId = -1;

	//	/**
	//	 * @brief 管理スレッド数
	//	*/
	//	static inline s8 mThreadCounter = 0;

	//	/**
	//	 * @brief スレッド名
	//	*/
	//	static inline thread_local std::wstring_view mThreadName = L"Unknown";

	//	/**
	//	 * @brief スレッドID
	//	*/
	//	s8 mThreadId;

	//	/**
	//	 * @brief スレッドの状態
	//	*/
	//	ThreadState mThreadState;

	//	/**
	//	 * @brief 優先度
	//	*/
	//	ThreadPriority mThreadPriority;

	//	/**
	//	 * @brief スタックサイズ
	//	*/
	//	u32 mStackSize;

	//	/**
	//	 * @brief スレッド実行関数
	//	*/
	//	std::function<void()> mThreadFunc;

	//	/**
	//	 * @brief スレッド終了時の関数テーブル
	//	*/
	//	static inline array<std::function<void()>, MAX_THREAD_ID> mTerminateFuncTable = { nullptr };
	//};


}