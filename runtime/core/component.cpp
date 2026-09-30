//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	component.cpp
///	@brief	component
#include	"pch.h"
#include	"component.h"

namespace nox
{
	namespace 
	{
		struct IComp {};

		template<class T>
		struct ComponentExBase : public IComp
		{
			constexpr ComponentExBase() noexcept([]()constexpr noexcept->bool {return true;}())
				= default;
		};

		struct ComponentEx : public ComponentExBase<ComponentEx>
		{

		};

		inline void test()
		{

		}
	}
}