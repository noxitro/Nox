//	Copyright (C) 2024 NOX ENGINE All Rights Rserved.

///	@file	atomic.h
///	@brief	atomic
#pragma once
#include	"../basic_definition.h"

#if NOX_WIN64
#include	"detail/atomic_win64.h"
#else
static_assert(false);
#endif // NOX_WIN64

namespace nox::os
{
	template<class T>
	struct Atomic
	{
		inline T Increment()
		{
			return nox::os::atomic::Increment(value_);
		}

		inline T Decrement()
		{
			return nox::os::atomic::Decrement(value_);
		}


	private:
		T value_;
	};
}