//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	read_write_lock_win64.h
///	@brief	read_write_lock_win64
#pragma once
#include    "../basic_definition.h"

#if NOX_WINDOWS
#pragma warning(push)
#pragma warning(disable: 26110)
namespace nox::detail
{
	class ReadWriteLockWin64
	{
	public:
		constexpr ReadWriteLockWin64() noexcept :
			lock_(nullptr)
		{
		}

		inline constexpr ~ReadWriteLockWin64()noexcept = default;

        void LockExclusive()noexcept;

        void UnlockExclusive() noexcept;

        bool TryLockExclusive() noexcept;

        // 共有ロック（読み込み）
        void LockShared() noexcept;

        void UnlockShared() noexcept;

        bool TryLockShared() noexcept;

	private:
        void* lock_;
	};
}
#pragma warning(pop)
#endif // NOX_WINDOWS
