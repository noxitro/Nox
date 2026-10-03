//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	mutex_win64.h
///	@brief	mutex_win64
#pragma once
#include	"../mutex.h"
#include	"../win64_api.h"

namespace nox
{
//	class Mutex;
}

struct nox::detail::MutexDetail final
{
	static constexpr ::PSRWLOCK GetNativeLock(const nox::Mutex& mutex) noexcept
	{
		return static_cast<::PSRWLOCK>(mutex.lock_);
	}
};

namespace nox::detail
{
	constexpr ::PSRWLOCK GetNativeLock(const nox::Mutex& mutex)
	{
		return nox::detail::MutexDetail::GetNativeLock(mutex);
	}
}
	