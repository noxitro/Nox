#pragma once

#include	"basic_type.h"
#include	<source_location>
#include	<concepts>
#include	<array>
#include	<string_view>
#include	<string>
#include	<vector>
#include    <span>
#include	<list>
#include	<deque>
#include	<queue>
#include	<set>
#include	<map>
#include	<unordered_map>
#include	<functional>
#include	<stack>
#include	<bitset>

/// @brief NITRO ENGINEのnamespace
namespace nox
{
	/// @brief メモリ操作関係
	namespace memory
	{
		//	前方宣言
		template<class>
		class StlAllocateAdapter;
	}

	template<class T>
	using Vector = std::vector<T, memory::StlAllocateAdapter<T>>;

	template<class T>
	using List = std::list<T, memory::StlAllocateAdapter<T>>;

	template<class Key, class Value, class Hasher = std::hash<Key>, class Keyeq = std::equal_to<Key>>
	using UnorderedMap = std::unordered_map<Key, Value, Hasher, Keyeq, memory::StlAllocateAdapter<std::pair<const Key, Value>>>;

	template<class ValueType>
	using BasicString = std::basic_string< ValueType, std::char_traits<ValueType>, memory::StlAllocateAdapter<ValueType>>;

	using U8String = BasicString<c8>;
	using U16String = BasicString<c16>;
}