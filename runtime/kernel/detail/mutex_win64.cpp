// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

///	@file	mutex_x64.cpp
///	@brief	mutex_x64
#include	"pch.h"
#include	"mutex_win64.h"

::SRWLOCK* nox::Mutex::Detail::GetNativeLock(nox::Mutex& mutex) noexcept
{
	static_assert(sizeof(mutex.lock_) == sizeof(::SRWLOCK), "Mutex size mismatch.");
	//	lock_ 自体が ::SRWLOCK の実体 (ポインタ 1 個分) なので、そのアドレスを渡す
	return reinterpret_cast<::PSRWLOCK>(const_cast<void**>(&mutex.lock_));
}

void	nox::Mutex::Lock()noexcept
{
	::AcquireSRWLockExclusive(nox::Mutex::Detail::GetNativeLock(*this));
}

void	nox::Mutex::Unlock()noexcept
{
	::ReleaseSRWLockExclusive(nox::Mutex::Detail::GetNativeLock(*this));
}

bool	nox::Mutex::TryLock()noexcept
{
	return ::TryAcquireSRWLockExclusive(nox::Mutex::Detail::GetNativeLock(*this)) != FALSE;
}