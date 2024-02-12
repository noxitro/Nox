#pragma once

#include	"basic_type.h"
#include	<array>
#include	<bitset>
#include	<concepts>
#include	<deque>
#include	<functional>
#include	<list>
#include	<map>
#include	<queue>
#include	<set>
#include	<source_location>
#include    <span>
#include	<stack>
#include	<string>
#include	<string_view>
#include	<unordered_map>
#include	<vector>
#include	<utility>

#include	"../memory/stl_allocate_adapter.h"

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
	using List = std::list<T, memory::StlAllocateAdapter<T>>;

	template<class Key, class Value, class Hasher = std::hash<Key>, class Keyeq = std::equal_to<Key>>
	using UnorderedMap = std::unordered_map<Key, Value, Hasher, Keyeq, memory::StlAllocateAdapter<std::pair<const Key, Value>>>;

	template<class ValueType>
	using BasicString = std::basic_string< ValueType, std::char_traits<ValueType>, memory::StlAllocateAdapter<ValueType>>;

	using CString = BasicString<char>;

	/// @brief Narrow String
	using NString = BasicString<char>;

	/// @brief Wide String
	using WString = BasicString<wchar_t>;

	/// @brief UTF8 String
	using U8String = BasicString<char8>;
	using U16String = BasicString<char16>;
	using U32String = BasicString<char32>;

}