///	@file		type_traits.h
///	@brief		type_traits
#pragma once
#include    <type_traits>

#include	"../advanced_type.h"
#include	"../basic_type.h"

namespace nox
{
	/// @brief 非型テンプレートパラメータを推論するためのタグ
	/// @tparam Value 非型テンプレートパラメータ
	template<auto Value>
	struct NontypeTag
	{
		inline consteval explicit NontypeTag()noexcept = default;
	};

	template<auto Value>
	constexpr nox::NontypeTag<Value> Nontype{};

#pragma region ContainerElementType
	namespace detail
	{
		template<class T>
		struct ContainerElement
		{
			using type = T;
		};

		template<class T> requires(std::is_array_v<T>)
			struct ContainerElement<T>
		{
			using type = std::remove_extent_t<T>;
		};

		template<class T> requires(std::is_same_v<typename T::value_type, typename T::value_type> && !std::is_array_v<T>)
		struct ContainerElement<T>
		{
			using type = typename T::value_type;
		};
	}
	template<class T>
	using ContainerElementType = typename detail::ContainerElement<T>::type;

	
#pragma endregion

	template<class>
	constexpr bool IsTupleLikeValue = false;

	template<class... Types>
	constexpr bool IsTupleLikeValue<std::tuple<Types...>> = true;

	template<class Type1, class Type2>
	constexpr bool IsTupleLikeValue<std::pair<Type1, Type2>> = true;

	template<class T, size_t _Size>
	constexpr bool IsTupleLikeValue<std::array<T, _Size>> = true;

	/// @brief 文字列型か
	template<class T>
	constexpr bool IsCharTypeValue =
		std::is_same_v<T, char> ||
		std::is_same_v<T, signed char> ||
		std::is_same_v<T, unsigned char> ||
		std::is_same_v<T, wchar_t> ||
		std::is_same_v<T, char8_t> ||
		std::is_same_v<T, char16_t> ||
		std::is_same_v<T, char32_t>;


	/// @brief スコープ付きの列挙型か
#if __clang__

	template<class T>
	constexpr bool IsScopedEnumValue = false;

	template<class T> requires(std::is_enum_v<T>&& std::is_convertible_v<T, std::underlying_type_t<T>>)
		constexpr bool IsScopedEnumValue<T> = false;
#else
	template<class T>
	constexpr bool IsScopedEnumValue = std::is_scoped_enum_v<T>;
#endif // __clang__


	namespace detail
	{
		template<class T>
		constexpr nox::uint8 TemplateParamLength = 0;

		template<template<class...> class T, class... Args> requires(sizeof...(Args) > 0)
		constexpr nox::uint8 TemplateParamLength<T<Args...>> = sizeof...(Args);

		template<class T>
		struct IsVector : std::false_type {};

		template<class... Args>
		struct IsVector<std::vector<Args...>> : std::true_type {};

		template<class T>
		constexpr bool IsVectorV = IsVector<T>::value;

		template <typename T>
		struct IsStdArray : std::false_type {};

		template <typename T, std::size_t N>
		struct IsStdArray<std::array<T, N>> : std::true_type {};

		template<class T>
		constexpr bool IsStdArrayV = IsStdArray<T>::value;

		/// @brief 文字列型かどうか
		template<class T>
		struct is_string_class : ::std::false_type {};

		/// @brief std::basic_string型
		template<class ValueType, class TraitsType, class AllocatorType>
		struct is_string_class<::std::basic_string<ValueType, TraitsType, AllocatorType>>
			: ::std::true_type {
		};

		template<class T>
		struct IsStringViewClass : ::std::false_type {};

		/// @brief string_view型
		template<class ValueType, class TraitsType>
		struct IsStringViewClass<::std::basic_string_view<ValueType, TraitsType>> : ::std::true_type {};

		template<class T, class U>
		struct TupleCatType;

		template<class T, template<class...> class U, class... Args> requires(nox::IsTupleLikeValue<U<Args...>>)
			struct TupleCatType<T, U<Args...>>
		{
			using Type = std::tuple<T, Args...>;
		};

		template<template<class...> class T, class U, class... Args> requires(nox::IsTupleLikeValue<T<Args...>>)
			struct TupleCatType<T<Args...>, U>
		{
			using Type = std::tuple<Args..., U>;
		};

		template<template<class...> class T, template<class...> class U, class... TArgs, class... UArgs> requires(nox::IsTupleLikeValue<T<TArgs...>> && nox::IsTupleLikeValue<U<UArgs...>>)
			struct TupleCatType<T<TArgs...>, U<UArgs...>>
		{
			using Type = std::tuple<TArgs..., UArgs...>;
		};

		template<class T, class ArgsTuple>
		struct InvokeResultTypeWithTupleLikeImpl;

		template<class T, template<class...> class ArgsTuple, class... Args> 
			requires(
			nox::IsTupleLikeValue<ArgsTuple<Args...>>&&
			std::is_invocable_v<T, Args...>
				)
		struct InvokeResultTypeWithTupleLikeImpl<T, ArgsTuple<Args...>>
		{
			using Type = std::invoke_result_t<T, Args...>;
		};
	}

	template<class T, class Args>
	constexpr bool IsInvocableWithTupleLikeValue = false;

	template<class T, template<class...> class ArgsTuple, class... Args> requires(nox::IsTupleLikeValue<ArgsTuple<Args...>>)
	constexpr bool IsInvocableWithTupleLikeValue<T, ArgsTuple<Args...>> = std::is_invocable_v<T, Args...>;

	/// @brief tuple_likeな呼び出しの戻り値の型
	template<class T, class ArgsTuple> requires(nox::IsInvocableWithTupleLikeValue<T, ArgsTuple>)
	using InvokeResultTypeWithTupleLike = typename nox::detail::InvokeResultTypeWithTupleLikeImpl<T, ArgsTuple>::Type;

	/// @brief string型かどうか
	template<class T>
	constexpr bool IsStringClassValue = detail::is_string_class<T>::value;

	/// @brief string_view型かどうか
	template<class T>
	constexpr bool IsStringViewClassValue = detail::IsStringViewClass<T>::value;

	/// @brief 文字列型かどうか
	template<class T>
	constexpr bool IsStringClassAllValue = nox::IsStringClassValue<T> || nox::IsStringViewClassValue<T>;

	namespace detail
	{
		/// @brief 文字列関係の型から文字型を表す
		/// @tparam T 文字列関係の型
		template<class T>
		struct StringChar;

		/// @brief char type
		template<class T> requires(nox::IsCharTypeValue<std::decay_t<std::remove_pointer_t<std::decay_t<T>>>>)
			struct StringChar<T>
		{
			using type = std::decay_t<std::remove_pointer_t<std::decay_t<T>>>;
		};

		/// @brief string class
		template<class T> requires(nox::IsStringClassValue<std::decay_t<T>>)
			struct StringChar<T>
		{
			using type = typename std::decay_t<T>::value_type;
		};

		/// @brief string_view class
		template<class T> requires(nox::IsStringViewClassValue<std::decay_t<T>>)
			struct StringChar<T>
		{
			using type = typename std::decay_t<T>::value_type;
		};

	}


	/// @brief 文字列関係の型から文字型を表す
	/// @tparam T 文字列関係の型
	template<class T> requires(std::is_void_v<std::void_t<typename nox::detail::StringChar<T>::type>>)
	using StringCharType = typename nox::detail::StringChar<T>::type;

	template<class T, class U> requires(nox::IsTupleLikeValue<T> || nox::IsTupleLikeValue<U>)
	using TupleCatType = typename nox::detail::TupleCatType<T, U>::Type;

	/// @brief グローバル関数ポインタ型か
	/// @tparam T 型
	template<class T>
	constexpr bool IsGlobalFunctionPointerValue = 
		std::is_function_v<std::remove_pointer_t<std::decay_t<T>>> && std::is_member_function_pointer_v<T> == false;


	/// @brief  関数ポインタ型か
	template<class T>
	constexpr bool IsFunctionPointerValue = std::is_function_v<std::remove_pointer_t<T>>;

	template<class T>
	constexpr bool IsStaticFunctionPointerValue = std::is_function_v<std::remove_pointer_t<T>> && !std::is_member_function_pointer_v<T>;

	/// @brief sizeof可能な型かどうか
	template<typename T>
	constexpr bool IsSizeofTypeValue = false;

	template<typename T> requires(sizeof(T) >= 0)
	constexpr bool IsSizeofTypeValue<T> = true;

	/**
	 * @brief あらゆる関数型
	*/
	template<class T>
	constexpr bool IsEveryFunctionV =
		nox::IsFunctionPointerValue<T> ||
		std::is_function_v<T>;

	namespace concepts
	{
		/**
		 * @brief あらゆる関数型
		*/
		template<class T>
		concept EveryFunctionType = IsEveryFunctionV<T>;

		/// @brief 文字列型
		template<class T>
		concept Char = nox::IsCharTypeValue<T>;

		template<class T, class U>
		concept EqualityComparable = requires(const T & a, const U & b)
		{
			{ a == b } -> std::convertible_to<bool>;
		};

		template<class T>
		concept GlobalFunctionPointer = IsGlobalFunctionPointerValue<T>;

		template<class T>
		concept TupleLike = nox::IsTupleLikeValue<T>;
	}

	/// @brief const pointer型を表現する
	/// @tparam T 
	template<class T>
	using AddConstPointerType = std::conditional_t<
		std::is_pointer_v<T>,
		std::add_pointer_t<std::add_const_t<std::remove_pointer_t<T>>>,
		T>;

	/**
	 * @brief const pointerからconstを除去する
	*/
	template<class T>
	using RemoveConstPointerType = std::conditional_t<
		std::is_pointer_v<T>,
		std::add_pointer_t<std::remove_const_t<std::remove_pointer_t<T>>>,
		T>;

	/**
	 * @brief const lvalue reference型を表現する
	*/
	template<class T>
	using RemoveConstLvalueReferenceType = std::conditional_t<
		std::is_lvalue_reference_v<T>,
		std::add_lvalue_reference_t<std::remove_const_t<std::remove_reference_t<T>>>,
		T>;

	/**
	 * @brief lvalue reference型を表現する
	*/
	template<class T>
	using AddConstLvalueReferenceType =
		std::conditional_t<
		std::is_lvalue_reference_v<T>,
		std::add_lvalue_reference_t<std::add_const_t<std::remove_reference_t<T>>>,
		T>;

	/**
	 * @brief const rvalue reference型を表現する
	*/
	template<class T>
	using RemoveConstRvalueReferenceType = std::conditional_t<
		std::is_rvalue_reference_v<T>,
		std::add_rvalue_reference_t<std::remove_const_t<std::remove_reference_t<T>>>,
		T>;

	/**
	 * @brief rvalue reference型を表現する
	*/
	template<class T>
	using AddConstRvalueReferenceType =
		std::conditional_t<
		std::is_rvalue_reference_v<T>,
		std::add_rvalue_reference_t<std::add_const_t<std::remove_reference_t<T>>>,
		T>;

	/**
	 * @brief constを除去
	*/
	template<class T>
	using RemoveConstPointerReferenceType = RemoveConstRvalueReferenceType<RemoveConstLvalueReferenceType<RemoveConstPointerType<T>>>;

	/**
	 * @brief const pointer型かどうか
	*/
	template<class T>
	constexpr bool IsConstPointerValue = std::is_pointer_v<T> && std::is_same_v <T, AddConstPointerType<T>>;

	/**
	 * @brief const lvalue reference型かどうか
	*/
	template<class T>
	constexpr bool IsConstLvalueReferenceValue = std::is_lvalue_reference_v<T> && std::is_same_v<T, AddConstLvalueReferenceType<T>>;

	/**
	 * @brief const rvalue reference型かどうか
	*/
	template<class T>
	constexpr bool IsConstRvalueReferenceValue = std::is_rvalue_reference_v<T> && std::is_same_v<T, AddConstRvalueReferenceType<T>>;

	/// @brief あらゆるconst型か
	template<class T>
	constexpr bool IsConstAllValue = std::is_const_v<T> || IsConstPointerValue<T> || IsConstLvalueReferenceValue<T> || IsConstRvalueReferenceValue<T>;

	/// @brief シーケンスコンテナ
	template<class T>
	constexpr bool IsSequenceContainerClassValue = detail::IsStdArrayV<T> || detail::IsVectorV<T>;

}