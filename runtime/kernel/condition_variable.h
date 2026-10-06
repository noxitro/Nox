//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	condition_variable.h
///	@brief	condition_variable
#pragma once
#include	"basic_definition.h"

namespace nox
{
	class Mutex;

	class ConditionVariable final
	{
	public:
		constexpr ConditionVariable() noexcept :
			native_(nullptr)
		{
		}

		ConditionVariable(const ConditionVariable&)noexcept = delete;
		ConditionVariable(ConditionVariable&&)noexcept = delete;
		
		constexpr ~ConditionVariable() noexcept = default;

		void Wait(nox::Mutex& mutex)noexcept;
		void NotifyOne()noexcept;
		void NotifyAll()noexcept;

	private:
#if NOX_WINDOWS
		void* native_;
#else
		static_assert(false);
#endif // NOX_WINDOWS

	};
}