//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	read_write_lock_win64.cpp
///	@brief	read_write_lock_win64
#include	"pch.h"
#include    "../read_write_lock.h"

#include	"../win64_api.h"

static_assert(sizeof(void*) == sizeof(::SRWLOCK), "ReadWriteLock size mismatch.");

void nox::ReadWriteLock::LockExclusive()noexcept
{
    ::AcquireSRWLockExclusive(reinterpret_cast<::PSRWLOCK>(&lock_));
}

void nox::ReadWriteLock::UnlockExclusive() noexcept
{
    ::ReleaseSRWLockExclusive(reinterpret_cast<::PSRWLOCK>(&lock_));
}

bool nox::ReadWriteLock::TryLockExclusive() noexcept
{
    return ::TryAcquireSRWLockExclusive(reinterpret_cast<::PSRWLOCK>(&lock_)) != FALSE;
}

// 共有ロック（読み込み）
void nox::ReadWriteLock::LockShared() noexcept
{
    ::AcquireSRWLockShared(reinterpret_cast<::PSRWLOCK>(&lock_));
}

void nox::ReadWriteLock::UnlockShared() noexcept
{
    ::ReleaseSRWLockShared(reinterpret_cast<::PSRWLOCK>(&lock_));
}

bool nox::ReadWriteLock::TryLockShared() noexcept
{
    return ::TryAcquireSRWLockShared(reinterpret_cast<::PSRWLOCK>(&lock_)) != FALSE;
}