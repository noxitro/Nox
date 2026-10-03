//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	condition_variable_win64.cpp
///	@brief	condition_variable_win64
#include	"pch.h"
#include	"../condition_variable.h"

#include	"mutex_win64.h"
//#include	"../win64_api.h"

void nox::ConditionVariable::Wait(const nox::Mutex& mutex) noexcept
{
	static_assert(sizeof(nox::ConditionVariable::native_) == sizeof(::CONDITION_VARIABLE), "ConditionVariable size mismatch.");

	::SleepConditionVariableSRW(
		reinterpret_cast<::PCONDITION_VARIABLE>(&native_), 
		nox::detail::GetNativeLock(mutex),
		INFINITE,
		0
	);
}

void nox::ConditionVariable::NotifyOne() noexcept
{
	::WakeConditionVariable(reinterpret_cast<::PCONDITION_VARIABLE>(&native_));
}

void nox::ConditionVariable::NotifyAll() noexcept
{
	::WakeAllConditionVariable(reinterpret_cast<::PCONDITION_VARIABLE>(&native_));
}