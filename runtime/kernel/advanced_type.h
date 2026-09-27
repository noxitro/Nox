// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

#pragma once
#include	<array>
#include	<bitset>
#include	<concepts>
#include	<deque>
#include	<functional>
#include	<list>
#include	<map>
#include    <optional>
#include	<queue>
#include	<set>
#include	<source_location>
#include    <span>
#include	<mdspan>
#include	<stack>
#include	<string>
#include	<string_view>
#include	<unordered_map>
#include	<unordered_set>
#include	<utility>
#include	<vector>
#include	<ranges>
#include	<expected>
#include	<chrono>
#include	<shared_mutex>
#include	<iostream>
#include	<fstream>
#include	<filesystem>
#include	<semaphore>

#include	"basic_type.h"
#include	"memory/stl_allocate_adapter.h"

/// @brief NITRO ENGINEのnamespace
namespace nox
{
	/// @brief メモリ操作関係
	//namespace memory
	//{
	//	//	前方宣言
	//	template<class>
	//	class StlAllocateAdapter;
	//}

	template<class T>
	using Vector = std::vector<T, memory::StlAllocateAdapter<T>>;

	template<class T>
	using Deque = std::deque<T, memory::StlAllocateAdapter<T>>;

	template<class T>
	using Queue = std::queue<T, Deque<T>>;

	template<class T>
	using List = std::list<T, memory::StlAllocateAdapter<T>>;

	template<class Key, class Value, class Hasher = std::hash<Key>, class Keyeq = std::equal_to<Key>>
	using UnorderedMap = std::unordered_map<Key, Value, Hasher, Keyeq, nox::memory::StlAllocateAdapter<std::pair<const Key, Value>>>;

	template<class Key, class Hasher = std::hash<Key>, class Keyeq = std::equal_to<Key>>
	using HashSet = std::unordered_set<Key, Hasher, Keyeq, nox::memory::StlAllocateAdapter<Key>>;

	template<class ValueType>
	using StlBasicString = std::basic_string< ValueType, std::char_traits<ValueType>, memory::StlAllocateAdapter<ValueType>>;

	using StlCString = StlBasicString<char>;
	using StlNString = StlBasicString<char>;
	using StlWString = StlBasicString<wchar_t>;
	using StlU8String = StlBasicString<char8>;
	using StlU16String = StlBasicString<char16>;
	using StlU32String = StlBasicString<char32>;

	namespace io
	{
		using u8ifstream = std::basic_ifstream<nox::char8, std::char_traits<nox::char8>>;
	}
}