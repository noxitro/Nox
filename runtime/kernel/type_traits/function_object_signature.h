//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	function_object_signature.h
///	@brief	関数オブジェクトのシグネチャ解析
#pragma once
#include	"type_traits.h"

namespace nox
{
	namespace detail
	{
		template<class T, class... Args>
		struct FunctionObjectSignature
		{
			using ResultType = std::invoke_result_t<T, Args...>;

			static	constexpr bool is_const = std::is_invocable_v<const T, Args...>;
			static	constexpr bool is_volatile = std::is_invocable_v<volatile T, Args...>;
			static	constexpr bool is_const_volatile = std::is_invocable_v<const volatile T, Args...>;
			static	constexpr bool is_lvalue_reference = std::is_invocable_v<T&, Args...>;
			static	constexpr bool is_const_lvalue_reference = std::is_invocable_v<const T&, Args...>;
			static	constexpr bool is_volatile_lvalue_reference = std::is_invocable_v<volatile T&, Args...>;
			static	constexpr bool is_const_volatile_lvalue_reference = std::is_invocable_v<const volatile T&, Args...>;
			static	constexpr bool is_rvalue_reference = std::is_invocable_v<T&&, Args...>;
			static	constexpr bool is_noexcept = noexcept(std::declval<T>()(std::declval<Args>()...));
		};

		template<class T, class ArgsTuple>
		struct FunctionObjectWithTupleLikeSignature;

		template<class T, template<class...> class ArgsTuple, class... Args> requires(nox::is_tuple_like_v<ArgsTuple<Args...>>)
		struct FunctionObjectWithTupleLikeSignature<T, ArgsTuple<Args...>> : nox::detail::FunctionObjectSignature<T, Args...> {};

		/*template<class T, class ArgsTuple>
		struct function_object_result_with_tuple_like_t;

		template<class T, template<class...> class ArgsTuple, class... Args> requires(nox::is_tuple_like_v< ArgsTuple<Args...>>)
			struct function_object_result_with_tuple_like_t<T, ArgsTuple<Args...>>
		{
			using type = typename nox::detail::FunctionObjectWithTupleLikeSignature<T, ArgsTuple<Args...>>::ResultType;
		};*/
	}

	/// @brief		関数オブジェクトか
	/// @details	is_class判定は修飾子を除去して反省する
	template<class T, class... Args>
	constexpr bool is_function_object_v = (std::is_class_v<std::decay_t<T>> || std::is_union_v<std::decay_t<T>>) && std::is_invocable_v<T, Args...>;

	/// @brief 関数オブジェクトか　tuple-like
	template<class T, class ArgsTuple>
	constexpr bool is_function_object_with_tuple_like_v = false;

	/// @brief 関数オブジェクトか　tuple-like
	template<class T, template<class...> class ArgsTuple, class... Args> requires(nox::is_function_object_v<T, Args...>)
	constexpr bool is_function_object_with_tuple_like_v<T, ArgsTuple<Args...>> = true;

	template<class T, class... Args>
	constexpr bool is_function_object_const_v = false;

	template<class T, class... Args> requires(nox::is_function_object_v<T, Args...>)
	constexpr bool is_function_object_const_v<T, Args...> = nox::detail::FunctionObjectSignature<T, Args...>::is_const;

	template<class T, class ArgsTuple>
	constexpr bool is_function_object_const_with_tuple_like_v = false;
	template<class T, class ArgsTuple> requires(nox::is_function_object_with_tuple_like_v<T, ArgsTuple>)
	constexpr bool is_function_object_const_with_tuple_like_v<T, ArgsTuple> = nox::detail::FunctionObjectWithTupleLikeSignature<T, ArgsTuple>::is_const;

	template<class T, class ArgsTuple>
	constexpr bool is_function_object_noexcept_with_tuple_like_v = false;
	template<class T, class ArgsTuple> requires(nox::is_function_object_with_tuple_like_v<T, ArgsTuple>)
	constexpr bool is_function_object_noexcept_with_tuple_like_v<T, ArgsTuple> = nox::detail::FunctionObjectWithTupleLikeSignature<T, ArgsTuple>::is_noexcept;

	/// @brief 関数オブジェクトの戻り値の型
	template<class T, class... Args> requires(nox::is_function_object_v<T, Args...>)
	using function_object_result_t = typename nox::detail::FunctionObjectSignature<T, Args...>::ResultType;

	/// @brief 関数オブジェクトの戻り値の型 tuple like
	template<class T, class ArgsTuple> requires(nox::is_function_object_with_tuple_like_v<T, ArgsTuple>)
	using function_object_result_with_tuple_like_t = typename nox::detail::FunctionObjectWithTupleLikeSignature<T, ArgsTuple>::ResultType;

	//template<class T, class ArgsTuple>
	//using function_object_result_with_tuple_like_t = typename nox::detail::function_object_result_with_tuple_like_t<T, ArgsTuple>::type;
}