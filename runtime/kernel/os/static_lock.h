// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

///	@file	static_lock.h
///	@brief	静的初期化中/静的デストラクタ後でも使えるロック
#pragma once
#include	"windows.h"
#include	"detail/mutex_base.h"

#if NOX_WIN64
namespace nox::os
{
	/// @brief		constinitで完成する排他ロック
	/// @details	nox::os::Mutex(::CRITICAL_SECTIONラッパ)は動的初期化が必要なため、
	///				名前空間スコープに置くとそのTUの初期化子が走る前にロックした時点で
	///				未初期化の::CRITICAL_SECTIONを踏んでアクセス違反になる。
	///				静的デストラクタ(DeleteCriticalSection)が走った後のロックも同様。
	///				::SRWLOCKはSRWLOCK_INITが{0}でゼロ初期化のみで完成し、初期化関数も
	///				破棄処理も要らないため、constinitで宣言すればプロセスのどの時点でも
	///				安全に取得できる。定常状態のコストも::CRITICAL_SECTIONより増えない。
	///				MEMO:	::SRWLOCKの排他モードは再帰取得できない(同一スレッドで
	///						二重にロックするとデッドロックする)。ロック区間から
	///						自己再入し得る用途にはこの型を使ってはいけない。
	///	@warning	名前空間スコープに置く場合は必ずconstinitを付けること。
	class StaticLock final : public nox::os::detail::MutexBase
	{
	public:
		inline constexpr StaticLock()noexcept = default;
		inline ~StaticLock()noexcept = default;

		StaticLock(const StaticLock&) = delete;
		StaticLock(StaticLock&&) = delete;
		StaticLock& operator=(const StaticLock&) = delete;
		StaticLock& operator=(StaticLock&&) = delete;

		inline	void	Lock()noexcept { ::AcquireSRWLockExclusive(&lock_); }

		inline	void	Unlock()noexcept { ::ReleaseSRWLockExclusive(&lock_); }

		[[nodiscard]] inline	bool	TryLock()noexcept { return ::TryAcquireSRWLockExclusive(&lock_) != 0; }

	private:
		/// @brief ゼロ初期化のみで完成するロック実体
		::SRWLOCK	lock_ = SRWLOCK_INIT;
	};
}
#else
	static_assert(false);
#endif // NOX_WIN64
