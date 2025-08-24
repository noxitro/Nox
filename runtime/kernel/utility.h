//	Copyright (C) 2025 NOX ENGINE All rights reserved.

///	@file	utility.h
///	@brief	utility
#pragma once

namespace nox
{
	class ScopeExit
	{
	public:
		inline constexpr explicit ScopeExit(void(* const func)())noexcept :
			func_(func)
		{
		}

		inline ~ScopeExit()
		{
			func_();
		}

	private:
		void(* const func_)();
	};
}