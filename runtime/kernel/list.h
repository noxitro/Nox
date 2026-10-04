//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	list.h
///	@brief	list
#pragma once
#include	<list>
#include	"memory/stl_allocate_adapter.h"

namespace nox
{
	template<class T>
	using List = std::list<T, nox::memory::StlAllocateAdapter<T>>;
}
