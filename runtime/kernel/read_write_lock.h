//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	read_write_lock.h
///	@brief	read_write_lock
#pragma once
#include	"basic_type.h"

namespace nox
{ 
	class ReadWriteLock final
	{
	public:
		inline constexpr ReadWriteLock()noexcept:
			lock_(nullptr),
			read_count_(0)
		{

		}

		void EnterReadLock()noexcept;
		void ExitReadLock()noexcept;
		void EnterWriteLock()noexcept;
		void ExitWriteLock()noexcept;
	private:
		//	各APIで実装する
		void LockExclusive()noexcept;
		void UnlockExclusive() noexcept;
		bool TryLockExclusive() noexcept;
		// 共有ロック（読み込み）
		void LockShared() noexcept;
		void UnlockShared() noexcept;
		bool TryLockShared() noexcept;
	private:
		void* lock_;
		nox::uint32 read_count_;
	};

	template<class _LockType>
		struct ScopedReadLock final
	{
		inline explicit ScopedReadLock(_LockType& lock) noexcept(noexcept(lock.EnterReadLock())) :
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
	struct ScopedWriteLock final
	{
		inline explicit ScopedWriteLock(_LockType& lock) noexcept(noexcept(lock.EnterWriteLock())) :
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