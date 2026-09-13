// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

///	@file	atomic_x64.h
///	@brief	atomic_x64
#pragma once
#include	"../../basic_definition.h"
#if NOX_WIN64
#include	"../windows.h"
#include	"../os_definition.h"

namespace nox::os
{
	namespace atomic
	{
		//	increment
		template<std::integral T> requires(sizeof(T) == sizeof(char))
			inline T Increment(volatile T& value) noexcept
		{
			return static_cast<T>(::_InterlockedExchangeAdd8(
				reinterpret_cast<volatile char*>(&value), 1) + 1);
		}

		template<std::integral T> requires(sizeof(T) == sizeof(long))
			inline T Increment(volatile T& value) noexcept
		{
			return InterlockedIncrement(reinterpret_cast<volatile long*>(&value));
		}

		template<std::integral T> requires(sizeof(T) == sizeof(short))
			inline T Increment(volatile T& value) noexcept
		{
			return InterlockedIncrement16(reinterpret_cast<volatile short*>(&value));
		}

		template<std::integral T> requires(sizeof(T) == sizeof(__int64))
			inline T Increment(volatile T& value) noexcept
		{
			return InterlockedIncrement64(static_cast<volatile __int64*>(&value));
		}

		//	decrement
		template<std::integral T> requires(sizeof(T) == sizeof(char))
			inline T Decrement(volatile T& value) noexcept
		{
			return static_cast<T>(::_InterlockedExchangeAdd8(
				reinterpret_cast<volatile char*>(&value), -1) - 1);
		}

		template<std::integral T> requires(sizeof(T) == sizeof(long))
			inline T Decrement(volatile T& value) noexcept
		{
			return InterlockedDecrement(reinterpret_cast<volatile long*>(&value));
		}

		template<std::integral T> requires(sizeof(T) == sizeof(short))
			inline T Decrement(volatile T& value) noexcept
		{
			return InterlockedDecrement16(reinterpret_cast<volatile short*>(&value));
		}

		template<std::integral T> requires(sizeof(T) == sizeof(__int64))
			inline T Decrement(volatile T& value) noexcept
		{
			return InterlockedDecrement64(static_cast<volatile __int64*>(&value));
		}

		//	read
		template<std::integral T> requires(sizeof(T) == sizeof(long))
			inline T Read(volatile T& value) noexcept
		{
			return InterlockedCompareExchange(reinterpret_cast<volatile long*>(&value), 0, 0);
		}

		template<std::integral T> requires(sizeof(T) == sizeof(short))
			inline T Read(volatile T& value) noexcept
		{
			return InterlockedCompareExchange(reinterpret_cast<volatile short*>(&value), 0, 0);
		}

		template<std::integral T> requires(sizeof(T) == sizeof(__int64))
			inline T Read(volatile T& value) noexcept
		{
			return InterlockedCompareExchange(static_cast<volatile __int64*>(&value), 0, 0);
		}
	}
}
#endif // NOX_WIN64