//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	unordered_set.h
///	@brief	unordered_set
#pragma once
#include	<unordered_set>
#include	"memory/stl_allocate_adapter.h"

namespace nox
{
	template<class Key, class Hasher = std::hash<Key>, class Keyeq = std::equal_to<Key>>
	using HashSet = std::unordered_set<Key, Hasher, Keyeq, nox::memory::StlAllocateAdapter<Key>>;
}