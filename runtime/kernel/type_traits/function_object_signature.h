//	Copyright (C) 2024 NOX ENGINE All Rights Rserved.

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
			static	constexpr bool is_lvalue_ref = std::is_invocable_v<T&, Args...>;
			static	constexpr bool is_const_lvalue_ref = std::is_invocable_v<const T&, Args...>;
			static	constexpr bool is_volatile_lvalue_ref = std::is_invocable_v<volatile T&, Args...>;
			static	constexpr bool is_const_volatile_lvalue_ref = std::is_invocable_v<const volatile T&, Args...>;
			static	constexpr bool is_rvalue_ref = std::is_invocable_v<T&&, Args...>;
			static	constexpr bool is_noexcept = noexcept(std::declval<T>()(std::declval<Args>()...));
		};

		template<class T, class ArgsTuple>
		struct FunctionObjectWithTupleLikeSignature;

		template<class T, template<class...> class ArgsTuple, class... Args> requires(nox::IsTupleLikeValue<ArgsTuple<Args...>>)
		struct FunctionObjectWithTupleLikeSignature<T, ArgsTuple<Args...>> : nox::detail::FunctionObjectSignature<T, Args...> {};

		/*template<class T, class ArgsTuple>
		struct FunctionObjectResultTypeWithTupleLike;

		template<class T, template<class...> class ArgsTuple, class... Args> requires(nox::IsTupleLikeValue< ArgsTuple<Args...>>)
			struct FunctionObjectResultTypeWithTupleLike<T, ArgsTuple<Args...>>
		{
			using type = typename nox::detail::FunctionObjectWithTupleLikeSignature<T, ArgsTuple<Args...>>::ResultType;
		};*/
	}

	/// @brief		関数オブジェクトか
	/// @details	is_class判定は修飾子を除去して反省する
	template<class T, class... Args>
	constexpr bool IsFunctionObjectValue = (std::is_class_v<std::decay_t<T>> || std::is_union_v<std::decay_t<T>>) && std::is_invocable_v<T, Args...>;

	/// @brief 関数オブジェクトか　tuple-like
	template<class T, class ArgsTuple>
	constexpr bool IsFunctionObjectWithTupleLikeValue = false;

	/// @brief 関数オブジェクトか　tuple-like
	template<class T, template<class...> class ArgsTuple, class... Args> requires(nox::IsFunctionObjectValue<T, Args...>)
	constexpr bool IsFunctionObjectWithTupleLikeValue<T, ArgsTuple<Args...>> = true;

	template<class T, class... Args>
	constexpr bool IsFunctionObjectConstValue = false;

	template<class T, class... Args> requires(nox::IsFunctionObjectValue<T, Args...>)
	constexpr bool IsFunctionObjectConstValue<T, Args...> = nox::detail::FunctionObjectSignature<T, Args...>::is_const;

	template<class T, class ArgsTuple>
	constexpr bool IsFunctionObjectConstWithTupleLikeValue = false;
	template<class T, class ArgsTuple> requires(nox::IsFunctionObjectWithTupleLikeValue<T, ArgsTuple>)
	constexpr bool IsFunctionObjectConstWithTupleLikeValue<T, ArgsTuple> = nox::detail::FunctionObjectWithTupleLikeSignature<T, ArgsTuple>::is_const;

	template<class T, class ArgsTuple>
	constexpr bool IsFunctionObjectNoexceptWithTupleLikeValue = false;
	template<class T, class ArgsTuple> requires(nox::IsFunctionObjectWithTupleLikeValue<T, ArgsTuple>)
	constexpr bool IsFunctionObjectNoexceptWithTupleLikeValue<T, ArgsTuple> = nox::detail::FunctionObjectWithTupleLikeSignature<T, ArgsTuple>::is_noexcept;

	/// @brief 関数オブジェクトの戻り値の型
	template<class T, class... Args> requires(nox::IsFunctionObjectValue<T, Args...>)
	using FunctionObjectResultType = typename nox::detail::FunctionObjectSignature<T, Args...>::ResultType;

	/// @brief 関数オブジェクトの戻り値の型 tuple like
	template<class T, class ArgsTuple> requires(nox::IsFunctionObjectWithTupleLikeValue<T, ArgsTuple>)
	using FunctionObjectResultTypeWithTupleLike = typename nox::detail::FunctionObjectWithTupleLikeSignature<T, ArgsTuple>::ResultType;

	//template<class T, class ArgsTuple>
	//using FunctionObjectResultTypeWithTupleLike = typename nox::detail::FunctionObjectResultTypeWithTupleLike<T, ArgsTuple>::type;
}