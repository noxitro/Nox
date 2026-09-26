//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	read_write_lock_win64.h
///	@brief	read_write_lock_win64
#pragma once
#include	"../windows.h"

#if NOX_WINDOWS
#pragma warning(push)
#pragma warning(disable: 26110)
namespace nox::os::detail
{
	class ReadWriteLockWin64
	{
	public:
		inline constexpr ReadWriteLockWin64() noexcept:
			lock_(SRWLOCK_INIT)
		{
		}

		inline void LockExclusive()noexcept
		{
			::AcquireSRWLockExclusive(&lock_);
		}

        inline void UnlockExclusive() noexcept
        {
            ::ReleaseSRWLockExclusive(&lock_);
        }

        inline bool TryLockExclusive() noexcept
        {
            return ::TryAcquireSRWLockExclusive(&lock_) != FALSE;
        }

        // 共有ロック（読み込み）
        inline void LockShared() noexcept
        {
            ::AcquireSRWLockShared(&lock_);
        }

        inline void UnlockShared() noexcept
        {
            ::ReleaseSRWLockShared(&lock_);
        }

        inline bool TryLockShared() noexcept
        {
            return ::TryAcquireSRWLockShared(&lock_) != FALSE;
        }

	private:
		::SRWLOCK lock_;
	};
}
#pragma warning(pop)
#endif // NOX_WINDOWS
