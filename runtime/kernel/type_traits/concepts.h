///	@file	concepts.h
///	@brief	他に依存しないconcept群
#pragma once
#include	<type_traits>
#include	<concepts>

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

	/// @brief メンバ変数ポインタ
	template<class T>
	concept MemberObjectPointer = std::is_member_object_pointer_v<T>;

	template<class T>
	concept Class = std::is_class_v<T>;

	/// @brief class or union
	template<class T>
	concept ClassOrUnion = std::is_class_v<T> || std::is_union_v<T>;

	template<class T>
	concept Array = std::is_array_v<T>;


	namespace detail
	{
		template<class T>
		concept HasDefaultOperator = requires(T & x) {
			x.operator=(x);
		};

		template<class T>
		concept HasIndexOperator = requires(T & x) {
			x.operator[](0);
		};

		template<class T>
		concept HasEqualityCompare = requires(T & x)
		{
			{ x.operator==(x) }->std::same_as<bool>;
		};

		template<class T>
		concept HasOperatorArrow = requires(T && t)
		{
			t.operator->();
		};
	}


	template<class T>
	constexpr bool IsInvokableDefaultOperatorValue = nox::concepts::detail::HasDefaultOperator<T>;

	template<class T>
	constexpr bool HasIndexOperatorValue = nox::concepts::detail::HasIndexOperator<T>;

	template<class T>
	constexpr bool HasEqualityCompareValue = nox::concepts::detail::HasEqualityCompare<T>;

}