///	@file	concepts.h
///	@brief	concepts
#pragma once
#include	<type_traits>

namespace nox::concepts
{
	template<class T>
	concept Pointer = std::is_pointer_v<T>;

	/// @brief Enum型
	template<class T>
	concept Enum = std::is_enum<T>::value;

	/// @brief メンバ関数ポインタ
	template<class T>
	concept MemberFunctionPointer = std::is_member_function_pointer_v<T>;

	/// @brief 算術型
	template<class T>
	concept Arithmetic = std::is_arithmetic_v<T>;

	template<class T>
	concept MemberObjectPointer = std::is_member_object_pointer_v<T>;

	/// @brief class or union
	template<class T>
	concept UserDefinedCompoundType = std::is_class_v<T> || std::is_union_v<T>;

	template<class T>
	concept Array = std::is_array_v<T>;
}