// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

///	@file	mutex_x64.cpp
///	@brief	mutex_x64
#include	"pch.h"
#include	"mutex_win64.h"


void	nox::Mutex::Lock()noexcept
{
	::AcquireSRWLockExclusive(nox::detail::GetNativeLock(*this));
}

void	nox::Mutex::Unlock()noexcept
{
	::ReleaseSRWLockExclusive(nox::detail::GetNativeLock(*this));
}

bool	nox::Mutex::TryLock()noexcept
{
	return ::TryAcquireSRWLockExclusive(nox::detail::GetNativeLock(*this)) != FALSE;
}