//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	read_write_lock_win64.cpp
///	@brief	read_write_lock_win64
#include	"pch.h"
#include	"read_write_lock_win64.h"

#include	"../win64_api.h"

static_assert(sizeof(void*) == sizeof(::SRWLOCK), "ReadWriteLock size mismatch.");

void nox::detail::ReadWriteLockWin64::LockExclusive()noexcept
{
    ::AcquireSRWLockExclusive(reinterpret_cast<::PSRWLOCK>(&lock_));
}

void nox::detail::ReadWriteLockWin64::UnlockExclusive() noexcept
{
    ::ReleaseSRWLockExclusive(reinterpret_cast<::PSRWLOCK>(&lock_));
}

bool nox::detail::ReadWriteLockWin64::TryLockExclusive() noexcept
{
    return ::TryAcquireSRWLockExclusive(reinterpret_cast<::PSRWLOCK>(&lock_)) != FALSE;
}

// 共有ロック（読み込み）
void nox::detail::ReadWriteLockWin64::LockShared() noexcept
{
    ::AcquireSRWLockShared(reinterpret_cast<::PSRWLOCK>(&lock_));
}

void nox::detail::ReadWriteLockWin64::UnlockShared() noexcept
{
    ::ReleaseSRWLockShared(reinterpret_cast<::PSRWLOCK>(&lock_));
}

bool nox::detail::ReadWriteLockWin64::TryLockShared() noexcept
{
    return ::TryAcquireSRWLockShared(reinterpret_cast<::PSRWLOCK>(&lock_)) != FALSE;
}