// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

///	@file	mutex_x64.cpp
///	@brief	mutex_x64
#include	"pch.h"
#include	"mutex_win64.h"


void	nox::Mutex::Lock()noexcept
{
	::AcquireSRWLockExclusive(static_cast<::PSRWLOCK>(lock_));
}

void	nox::Mutex::Unlock()noexcept
{
	::ReleaseSRWLockExclusive(static_cast<::PSRWLOCK>(lock_));
}

bool	nox::Mutex::TryLock()noexcept
{
	return ::TryAcquireSRWLockExclusive(static_cast<::PSRWLOCK>(lock_)) != FALSE;
}