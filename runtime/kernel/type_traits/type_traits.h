// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

///	@file		type_traits.h
///	@brief		type_traits
#pragma once
#include    <type_traits>
#include    <array>
#include    <concepts>
#include    <string>
#include    <string_view>
#include    <utility>
#include    <vector>

#include	"../advanced_type.h"
#include	"../basic_type.h"
#include    "../basic_definition.h"

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


	/// @brief new可能か
	/// @tparam T 
	template<class T>
	inline constexpr bool is_newable_v =
		requires { new T(); };

	/// @brief nothrow new可能か
	/// @tparam T 
	template<class T>
	inline constexpr bool is_nothrow_newable_v =
		requires { { new T() } noexcept; };

	template<class T>
	inline constexpr bool is_deletable_v =
		requires(T * ptr) { delete ptr; };

#pragma region container_element_t
	namespace detail
	{
		template<class T>
		struct container_element
		{
			using type = T;
		};

		template<class T> requires(std::is_array_v<T>)
			struct container_element<T>
		{
			using type = std::remove_extent_t<T>;
		};

		template<class T> requires(std::is_same_v<typename T::value_type, typename T::value_type> && !std::is_array_v<T>)
		struct container_element<T>
		{
			using type = typename T::value_type;
		};
	}
	template<class T>
	using container_element_t = typename detail::container_element<std::remove_reference_t<T>>::type;

	
#pragma endregion

	template<class>
	constexpr bool is_tuple_like_v = false;

	template<class... Types>
	constexpr bool is_tuple_like_v<std::tuple<Types...>> = true;

	template<class Type1, class Type2>
	constexpr bool is_tuple_like_v<std::pair<Type1, Type2>> = true;

	template<class T, size_t _Size>
	constexpr bool is_tuple_like_v<std::array<T, _Size>> = true;

	/// @brief 文字列型か
	template<class T>
	constexpr bool is_char_v =
		std::is_same_v<T, char> ||
		std::is_same_v<T, signed char> ||
		std::is_same_v<T, unsigned char> ||
		std::is_same_v<T, wchar_t> ||
		std::is_same_v<T, char8_t> ||
		std::is_same_v<T, char16_t> ||
		std::is_same_v<T, char32_t>;


	/// @brief スコープ付きの列挙型か
#if NOX_COMPILER_CLANG

	template<class T>
	constexpr bool is_scoped_enum_v = false;

	template<class T> requires(std::is_enum_v<T>&& std::is_convertible_v<T, std::underlying_type_t<T>>)
		constexpr bool is_scoped_enum_v<T> = false;
#else
	template<class T>
	constexpr bool is_scoped_enum_v = std::is_scoped_enum_v<T>;
#endif // __clang__


	namespace detail
	{
		template<class T>
		constexpr nox::uint8 template_param_length_v = 0;

		template<template<class...> class T, class... Args> requires(sizeof...(Args) > 0)
		constexpr nox::uint8 template_param_length_v<T<Args...>> = sizeof...(Args);

		template<class T>
		struct is_vector : std::false_type {};

		template<class... Args>
		struct is_vector<std::vector<Args...>> : std::true_type {};

		template<class T>
		constexpr bool is_vector_v = is_vector<T>::value;

		template <typename T>
		struct is_std_array : std::false_type {};

		template <typename T, std::size_t N>
		struct is_std_array<std::array<T, N>> : std::true_type {};

		template<class T>
		constexpr bool is_std_array_v = is_std_array<T>::value;

		/// @brief 文字列型かどうか
		template<class T>
		struct is_string_class : ::std::false_type {};

		/// @brief std::basic_string型
		template<class ValueType, class TraitsType, class AllocatorType>
		struct is_string_class<::std::basic_string<ValueType, TraitsType, AllocatorType>>
			: ::std::true_type {
		};

		template<class T>
		struct is_string_view_class : ::std::false_type {};

		/// @brief string_view型
		template<class ValueType, class TraitsType>
		struct is_string_view_class<::std::basic_string_view<ValueType, TraitsType>> : ::std::true_type {};

		template<class T, class U>
		struct tuple_cat;

		template<class T, template<class...> class U, class... Args> requires(nox::is_tuple_like_v<U<Args...>>)
			struct tuple_cat<T, U<Args...>>
		{
			using type = std::tuple<T, Args...>;
		};

		template<template<class...> class T, class U, class... Args> requires(nox::is_tuple_like_v<T<Args...>>)
			struct tuple_cat<T<Args...>, U>
		{
			using type = std::tuple<Args..., U>;
		};

		template<template<class...> class T, template<class...> class U, class... TArgs, class... UArgs> requires(nox::is_tuple_like_v<T<TArgs...>> && nox::is_tuple_like_v<U<UArgs...>>)
			struct tuple_cat<T<TArgs...>, U<UArgs...>>
		{
			using type = std::tuple<TArgs..., UArgs...>;
		};

		template<class T, class ArgsTuple>
		struct invoke_result_with_tuple_like;

		template<class T, template<class...> class ArgsTuple, class... Args> 
			requires(
			nox::is_tuple_like_v<ArgsTuple<Args...>>&&
			std::is_invocable_v<T, Args...>
				)
		struct invoke_result_with_tuple_like<T, ArgsTuple<Args...>>
		{
			using type = std::invoke_result_t<T, Args...>;
		};
	}

	template<class T, class Args>
	constexpr bool is_invocable_with_tuple_like_v = false;

	template<class T, template<class...> class ArgsTuple, class... Args> requires(nox::is_tuple_like_v<ArgsTuple<Args...>>)
	constexpr bool is_invocable_with_tuple_like_v<T, ArgsTuple<Args...>> = std::is_invocable_v<T, Args...>;

	/// @brief tuple_likeな呼び出しの戻り値の型
	template<class T, class ArgsTuple> requires(nox::is_invocable_with_tuple_like_v<T, ArgsTuple>)
	using invoke_result_with_tuple_like_t = typename nox::detail::invoke_result_with_tuple_like<T, ArgsTuple>::type;

	/// @brief string型かどうか
	template<class T>
	constexpr bool is_string_class_v = detail::is_string_class<T>::value;

	/// @brief string_view型かどうか
	template<class T>
	constexpr bool is_string_view_class_v = detail::is_string_view_class<T>::value;

	/// @brief 文字列型かどうか
	template<class T>
	constexpr bool is_any_string_class_v = nox::is_string_class_v<T> || nox::is_string_view_class_v<T>;

	namespace detail
	{
		/// @brief 文字列関係の型から文字型を表す
		/// @tparam T 文字列関係の型
		template<class T>
		struct string_char;

		/// @brief char type
		template<class T> requires(nox::is_char_v<std::decay_t<std::remove_pointer_t<std::decay_t<T>>>>)
			struct string_char<T>
		{
			using type = std::decay_t<std::remove_pointer_t<std::decay_t<T>>>;
		};

		/// @brief string class
		template<class T> requires(nox::is_string_class_v<std::decay_t<T>>)
			struct string_char<T>
		{
			using type = typename std::decay_t<T>::value_type;
		};

		/// @brief string_view class
		template<class T> requires(nox::is_string_view_class_v<std::decay_t<T>>)
			struct string_char<T>
		{
			using type = typename std::decay_t<T>::value_type;
		};

	}


	/// @brief 文字列関係の型から文字型を表す
	/// @tparam T 文字列関係の型
	template<class T> requires requires { typename nox::detail::string_char<T>::type; }
	using string_char_t = typename nox::detail::string_char<T>::type;

	template<class T, class U> requires(nox::is_tuple_like_v<T> || nox::is_tuple_like_v<U>)
	using tuple_cat_t = typename nox::detail::tuple_cat<T, U>::type;

	/// @brief グローバル関数ポインタ型か
	/// @tparam T 型
	template<class T>
	constexpr bool is_global_function_pointer_v = 
		std::is_function_v<std::remove_pointer_t<std::decay_t<T>>> && std::is_member_function_pointer_v<T> == false;


	/// @brief  関数ポインタ型か
	template<class T>
	constexpr bool is_function_pointer_v = std::is_function_v<std::remove_pointer_t<T>>;

	template<class T>
	constexpr bool is_static_function_pointer_v = std::is_function_v<std::remove_pointer_t<T>> && !std::is_member_function_pointer_v<T>;

	/// @brief sizeof可能な型かどうか
	template<typename T>
	constexpr bool is_complete_v = false;

	template<typename T> requires(sizeof(T) >= 0)
	constexpr bool is_complete_v<T> = true;

	/**
	 * @brief あらゆる関数型
	*/
	template<class T>
	constexpr bool is_every_function_v =
		nox::is_function_pointer_v<T> ||
		std::is_function_v<T>;

	namespace concepts
	{
		/**
		 * @brief あらゆる関数型
		*/
		template<class T>
		concept EveryFunctionType = is_every_function_v<T>;

		/// @brief 文字列型
		template<class T>
		concept Char = nox::is_char_v<T>;

		template<class T, class U>
		concept EqualityComparable = requires(const T & a, const U & b)
		{
			{ a == b } -> std::convertible_to<bool>;
		};

		template<class T>
		concept GlobalFunctionPointer = is_global_function_pointer_v<T>;

		template<class T>
		concept TupleLike = nox::is_tuple_like_v<T>;
	}

	/// @brief const pointer型を表現する
	/// @tparam T 
	template<class T>
	using add_const_pointer_t = std::conditional_t<
		std::is_pointer_v<T>,
		std::add_pointer_t<std::add_const_t<std::remove_pointer_t<T>>>,
		T>;

	/**
	 * @brief const pointerからconstを除去する
	*/
	template<class T>
	using remove_const_pointer_t = std::conditional_t<
		std::is_pointer_v<T>,
		std::add_pointer_t<std::remove_const_t<std::remove_pointer_t<T>>>,
		T>;

	/**
	 * @brief const lvalue reference型を表現する
	*/
	template<class T>
	using remove_const_lvalue_reference_t = std::conditional_t<
		std::is_lvalue_reference_v<T>,
		std::add_lvalue_reference_t<std::remove_const_t<std::remove_reference_t<T>>>,
		T>;

	/**
	 * @brief lvalue reference型を表現する
	*/
	template<class T>
	using add_const_lvalue_reference_t =
		std::conditional_t<
		std::is_lvalue_reference_v<T>,
		std::add_lvalue_reference_t<std::add_const_t<std::remove_reference_t<T>>>,
		T>;

	/**
	 * @brief const rvalue reference型を表現する
	*/
	template<class T>
	using remove_const_rvalue_reference_t = std::conditional_t<
		std::is_rvalue_reference_v<T>,
		std::add_rvalue_reference_t<std::remove_const_t<std::remove_reference_t<T>>>,
		T>;

	/**
	 * @brief rvalue reference型を表現する
	*/
	template<class T>
	using add_const_rvalue_reference_t =
		std::conditional_t<
		std::is_rvalue_reference_v<T>,
		std::add_rvalue_reference_t<std::add_const_t<std::remove_reference_t<T>>>,
		T>;

	/**
	 * @brief constを除去
	*/
	template<class T>
	using remove_const_pointer_reference_t = remove_const_rvalue_reference_t<remove_const_lvalue_reference_t<remove_const_pointer_t<T>>>;

	/**
	 * @brief const pointer型かどうか
	*/
	template<class T>
	constexpr bool is_const_pointer_v = std::is_pointer_v<T> && std::is_same_v <T, add_const_pointer_t<T>>;

	/**
	 * @brief const lvalue reference型かどうか
	*/
	template<class T>
	constexpr bool is_const_lvalue_reference_v = std::is_lvalue_reference_v<T> && std::is_same_v<T, add_const_lvalue_reference_t<T>>;

	/**
	 * @brief const rvalue reference型かどうか
	*/
	template<class T>
	constexpr bool is_const_rvalue_reference_v = std::is_rvalue_reference_v<T> && std::is_same_v<T, add_const_rvalue_reference_t<T>>;

	/// @brief あらゆるconst型か
	template<class T>
	constexpr bool is_any_const_v = std::is_const_v<T> || is_const_pointer_v<T> || is_const_lvalue_reference_v<T> || is_const_rvalue_reference_v<T>;

	/// @brief シーケンスコンテナ
	template<class T>
	constexpr bool is_sequence_container_class_v = detail::is_std_array_v<T> || detail::is_vector_v<T>;

	namespace detail
	{
		template<class T, class = void>
		struct is_addressable : std::false_type {};

		template<class T>
		struct is_addressable<T, std::void_t<
			decltype(std::addressof(std::declval<std::remove_reference_t<T>&>()))
			>> : std::true_type {};
	}

	/// @brief &（address-of）でアドレス取得が可能か
	/// @note ビットフィールドは型では判別できないため、この特性は true になり得ます。
	///       実メンバのアドレス可否は bit 幅メタ情報と併用してください。
	template<class T>
	inline constexpr bool is_addressable_v = nox::detail::is_addressable<T>::value;

	namespace concepts
	{
		template<class T>
		concept Addressable = is_addressable_v<T>;
	}

	template<class... Types>
	struct type_identities_t {};
}