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
	static inline ::PSRWLOCK GetNativeLock(const nox::Mutex& mutex) noexcept
	{
		static_assert(sizeof(mutex.lock_) == sizeof(::SRWLOCK), "Mutex size mismatch.");
		//	lock_ 自体が ::SRWLOCK の実体 (ポインタ 1 個分) なので、そのアドレスを渡す
		return reinterpret_cast<::PSRWLOCK>(const_cast<void**>(&mutex.lock_));
	}
};

namespace nox::detail
{
	inline ::PSRWLOCK GetNativeLock(const nox::Mutex& mutex) noexcept
	{
		return nox::detail::MutexDetail::GetNativeLock(mutex);
	}
}
	