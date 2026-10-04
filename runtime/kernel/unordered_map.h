//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	unordered_map.h
///	@brief	unordered_map
#pragma once
#include	<unordered_map>
#include	"memory/stl_allocate_adapter.h"

namespace nox
{
	template<class Key, class Value, class Hasher = std::hash<Key>, class Keyeq = std::equal_to<Key>>
	using UnorderedMap = std::unordered_map<Key, Value, Hasher, Keyeq, nox::memory::StlAllocateAdapter<std::pair<const Key, Value>>>;
}