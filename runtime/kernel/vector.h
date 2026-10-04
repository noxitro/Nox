//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	vector.h
///	@brief	vector
#pragma once
#include	<vector>
#include	"memory/stl_allocate_adapter.h"

namespace nox
{
	template<class T>
	using Vector = std::vector<T, nox::memory::StlAllocateAdapter<T>>;
}