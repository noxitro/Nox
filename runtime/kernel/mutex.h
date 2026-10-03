// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

///	@file	mutex.h
///	@brief	mutex
#pragma once

namespace nox
{
	namespace detail
	{
		struct MutexDetail;
	}

	class Mutex final 
	{
		friend struct nox::detail::MutexDetail;
	public:
		inline constexpr Mutex()noexcept :
			lock_(nullptr) {
		}

		inline constexpr ~Mutex() noexcept = default;

		void	Lock() noexcept;
		void	Unlock() noexcept;
		bool	TryLock() noexcept;

	private:
		void* lock_;
	};
}