//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	read_write_lock.h
///	@brief	read_write_lock
#pragma once
#include	<type_traits>
#include	<concepts>
#include	"../basic_type.h"
#include	"../utility.h"
#include	"atomic.h"

#if NOX_WIN64
#include	"detail/read_write_lock_win64.h"
#else
static_assert(false);
#endif // NOX_WIN64

namespace nox::os
{ 
	namespace detail
	{
		template<class T>
		class ReadWriteLockImpl
		{
		public:
			inline constexpr ReadWriteLockImpl()noexcept:
				read_count_(0)
			{

			}

			inline void EnterReadLock()noexcept(noexcept(lock_.LockShared()))
			{
				nox::os::atomic::Increment(read_count_);
				lock_.LockShared();
			}

			inline void ExitReadLock()noexcept(noexcept(lock_.UnlockShared()))
			{
				nox::os::atomic::Decrement(read_count_);
				lock_.UnlockShared();
			}

			inline void EnterWriteLock()noexcept(noexcept(lock_.LockExclusive()))
			{
				lock_.LockExclusive();
			}

			inline void ExitWriteLock()noexcept(noexcept(lock_.UnlockExclusive()))
			{
				lock_.UnlockExclusive();
			}
		private:
			T lock_;
			nox::uint32 read_count_;
		};
	}

	using ReadWriteLock = nox::os::detail::ReadWriteLockImpl<nox::os::detail::ReadWriteLockWin64>;

	template<class _LockType>
		struct ScopedReadLock
	{
		inline ScopedReadLock(_LockType& lock) noexcept(noexcept(lock.EnterReadLock())) :
			lock_(lock)
		{
			lock_.EnterReadLock();
		}

		inline ~ScopedReadLock() noexcept(noexcept(lock_.ExitReadLock()))
		{
			lock_.ExitReadLock();
		}

	private:
		_LockType& lock_;
	};

	template<class _LockType>
	struct ScopedWriteLock
	{
		inline ScopedWriteLock(_LockType& lock) noexcept(noexcept(lock.EnterWriteLock())) :
			lock_(lock)
		{
			lock_.EnterWriteLock();
		}

		inline ~ScopedWriteLock() noexcept(noexcept(lock_.ExitWriteLock()))
		{
			lock_.ExitWriteLock();
		}

	private:
		_LockType& lock_;
	};
}