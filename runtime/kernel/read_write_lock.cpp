//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	read_write_lock.cpp
///	@brief	read_write_lock
#include	"pch.h"
#include	"read_write_lock.h"

#include	"atomic.h"

void nox::ReadWriteLock::EnterReadLock()noexcept
{
	nox::atomic::Increment(read_count_);
	LockShared();
}

void nox::ReadWriteLock::ExitReadLock()noexcept
{
	nox::atomic::Decrement(read_count_);
	UnlockShared();
}

void nox::ReadWriteLock::EnterWriteLock()noexcept
{
	LockExclusive();
}

void nox::ReadWriteLock::ExitWriteLock()noexcept
{
	UnlockExclusive();
}