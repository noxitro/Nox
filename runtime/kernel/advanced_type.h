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
	using StdBasicString = std::basic_string< ValueType, std::char_traits<ValueType>, memory::StlAllocateAdapter<ValueType>>;

	using StdCString = StdBasicString<char>;
	using StdNString = StdBasicString<char>;
	using StdWString = StdBasicString<wchar_t>;
	using StdU8String = StdBasicString<char8>;
	using StdU16String = StdBasicString<char16>;
	using StdU32String = StdBasicString<char32>;

}