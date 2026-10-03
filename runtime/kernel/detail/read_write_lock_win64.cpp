//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	read_write_lock_win64.cpp
///	@brief	read_write_lock_win64
#include	"pch.h"
#include	"read_write_lock_win64.h"

#include	"../win64_api.h"

void nox::detail::ReadWriteLockWin64::LockExclusive()noexcept
{
    ::AcquireSRWLockExclusive(static_cast<::SRWLOCK*>(lock_));
}

void nox::detail::ReadWriteLockWin64::UnlockExclusive() noexcept
{
    ::ReleaseSRWLockExclusive(static_cast<::SRWLOCK*>(lock_));
}

bool nox::detail::ReadWriteLockWin64::TryLockExclusive() noexcept
{
    return ::TryAcquireSRWLockExclusive(static_cast<::SRWLOCK*>(lock_)) != FALSE;
}

// 共有ロック（読み込み）
void nox::detail::ReadWriteLockWin64::LockShared() noexcept
{
    ::AcquireSRWLockShared(static_cast<::SRWLOCK*>(lock_));
}

void nox::detail::ReadWriteLockWin64::UnlockShared() noexcept
{
    ::ReleaseSRWLockShared(static_cast<::SRWLOCK*>(lock_));
}

bool nox::detail::ReadWriteLockWin64::TryLockShared() noexcept
{
    return ::TryAcquireSRWLockShared(static_cast<::SRWLOCK*>(lock_)) != FALSE;
}