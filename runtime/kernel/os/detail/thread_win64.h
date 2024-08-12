//	Copyright (C) 2024 NOX ENGINE All Rights Rserved.

///	@file	thread_win64.h
///	@brief	thread_win64
#pragma once
#include	"thread_base.h"

#include	"win64.h"

namespace nox::os::detail
{
	class ThreadWin64 : public ThreadBase
	{
	public:
		inline constexpr ThreadWin64()noexcept :
			native_thread_handle_(nullptr),
			native_thread_id_(0)
		{
		}

		~ThreadWin64()noexcept;

		/**
		 * @brief スレッド管理IDを取得
		 * @return
		*/
		static int8	GetThreadId() {
			//	まだアサインされていない
			if (current_thread_id_ < 0)
			{
				//	割り当て
				AssignThreadId();
			}

			return current_thread_id_;
		}

		/**
		 * @brief スレッドの実行
		 * @tparam FuncType 実行する関数の型
		 * @tparam ...Args 関数の引数群の型
		 * @param func
		 * @param ...args
		*/
		void Dispatch(const Delegate<void()>& func);

		/**
		 * @brief 停止
		*/
		void	Wait();

		/**
		 * @brief スレッド情報を取得
		 * @return スレッド情報
		*/
		static ThreadInfo GetThreadInfo();

		inline constexpr ::HANDLE GetNativeHandle()const noexcept { return native_thread_handle_; }
		inline constexpr uint32 GetNativeThreadId()const noexcept { return native_thread_id_; }
	private:
		/**
		 * @brief スレッド管理IDを割り当てる
		*/
		static void	AssignThreadId();

		/**
		 * @brief スレッドに登録するコールバック関数
		 * @param argPtr ThreadHandleWin64のアドレス
		 * @return エラーコード
		*/
		static inline uint32 CALLBACK ThreadProc(void* argPtr);
	private:
		//	static u8	MakeThreadId();
	private:
		/**
		 * @brief ネイティブスレッドハンドル
		*/
		::HANDLE native_thread_handle_;

		/**
		 * @brief ネイティブスレッドID
		*/
		uint32 native_thread_id_;

		static inline thread_local ThreadWin64* current_thread_ = nullptr;
	};
}