//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	queue.h
///	@brief	queue
#pragma once
#include	<queue>
#include	<deque>
#include	"memory/stl_allocate_adapter.h"

namespace nox
{
	template<class T>
	using Deque = std::deque<T, nox::memory::StlAllocateAdapter<T>>;

	template<class T>
	using Queue = std::queue<T, nox::Deque<T>>;
}