///	@file	atomic_x64.h
///	@brief	atomic_x64
#pragma once
#include	"../../if/basic_definition.h"
#if NOX_WIN64
#include	"../windows.h"
#include	"../os_definition.h"

namespace nox::os::atomic
{
	template<std::integral T> requires(sizeof(T) == sizeof(long))
	inline T Increment(volatile T& value)
	{
		return InterlockedIncrement(reinterpret_cast<volatile long*>(&value));
	}

	template<std::integral T> requires(sizeof(T) == sizeof(short))
		inline T Increment(volatile T& value)
	{
		return InterlockedIncrement16(static_cast<volatile short*>(&value));
	}

	template<std::integral T> requires(sizeof(T) == sizeof(__int64))
		inline T Increment(volatile T& value)
	{
		return InterlockedIncrement64(static_cast<volatile __int64*>(&value));
	}

	template<std::integral T> requires(sizeof(T) == sizeof(long))
		inline T Decrement(volatile T& value)
	{
		return InterlockedDecrement(reinterpret_cast<volatile long*>(&value));
	}

	template<std::integral T> requires(sizeof(T) == sizeof(short))
		inline T Decrement(volatile T& value)
	{
		return InterlockedDecrement16(static_cast<volatile short*>(&value));
	}

	template<std::integral T> requires(sizeof(T) == sizeof(__int64))
		inline T Decrement(volatile T& value)
	{
		return InterlockedDecrementt64(static_cast<volatile __int64*>(&value));
	}
}
#endif // NOX_WIN64