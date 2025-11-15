//	Copyright (C) 2025 NOX ENGINE All rights reserved.

///	@file	utility.h
///	@brief	utility
#pragma once
#include <functional>

namespace nox::util
{
	class ScopeExit
	{
	public:
		inline explicit ScopeExit(const std::function<void()>& func)noexcept :
			func_(func)
		{

		}

		inline ~ScopeExit()
		{
			func_();
		}

	private:
		std::function<void()> func_;
	};
}