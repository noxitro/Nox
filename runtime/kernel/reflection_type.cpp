//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	type.cpp
///	@brief	type
#include	"pch.h"
#include	"reflection_type.h"

namespace nox
{
	template<class T, class U, class... Tail>
	constexpr bool is_same_and_v=false;

	template<class T, class U>
	constexpr bool is_same_and_v<T, U> = std::is_same_v<T, U>;

	template<class T, class U, class... Tail> requires(sizeof...(Tail) > 0)
	constexpr bool is_same_and_v<T, U, Tail...> = std::is_same_v<T, U> && std::is_same_v<T, Tail...>;

	namespace detail
	{
		template<class T, class... Tail>
		struct is_same_or;

		template<class T>
		struct is_same_or<T>
		{
			static constexpr bool value = false;
		};

		template<class T, class U, class... Tail>
		struct is_same_or<T, U, Tail...>
		{
			static constexpr bool value = std::is_same_v<T, U> || is_same_or<T, Tail...>::value;
		};
	}

	template<class T, class U, class... Tail>
	constexpr bool is_same_or_v = nox::detail::is_same_or<T, U, Tail...>::value;
}

namespace nox::reflection
{

	inline constexpr bool IsConvertible(const nox::reflection::TypeKind a, const nox::reflection::TypeKind b)noexcept
	{


		if (a == TypeKind::Char && b == TypeKind::Char)
		{
			return true;
		}

		if (a == TypeKind::Char16 && b == TypeKind::Char16)
		{
			return true;
		}

		if (a == TypeKind::Char32 && b == TypeKind::Char32)
		{
			return true;
		}

		if (a == TypeKind::WideChar && b == TypeKind::WideChar)
		{
			return true;
		}

		if (a == TypeKind::Int8 && b == TypeKind::Int8)
		{
			return true;
		}

		if (a == TypeKind::UInt8 && b == TypeKind::UInt8)
		{
			return true;
		}

		return false;
	}
}


namespace nox
{
	//	暗黙変換可能かを判定する関数が正しく機能しているかテスト

	template<class From, class To>
	inline constexpr bool CheckImpl()noexcept
	{
		return std::is_convertible_v<From, To> ==
			nox::reflection::Typeof<From>().IsConvertible(nox::reflection::Typeof<To>());
	}

	template<class From, class To>
	constexpr bool Check()noexcept
	{
		return nox::CheckImpl<From, To>() && nox::CheckImpl<To, From>();
	}

	template<class T>
	inline constexpr bool CheckGroup()noexcept
	{
	/*	using AddLRef = std::add_lvalue_reference_t<T>;
		using AddRRef = std::add_rvalue_reference_t<T>;
		using AddConst = std::add_const_t<T>;
		using AddVolatile = std::add_volatile_t<T>;
		using AddConstLRef = std::add_lvalue_reference_t<std::add_const_t<T>>;
		using AddConstRRef = std::add_rvalue_reference_t<std::add_const_t<T>>;
		using AddConstVolatile = std::add_volatile_t<std::add_const_t<T>>;
		using AddConstLRefVolatile = std::add_volatile_t<std::add_lvalue_reference_t<std::add_const_t<T>>>;
		using AddConstRRefVolatile = std::add_volatile_t<std::add_rvalue_reference_t<std::add_const_t<T>>>;
		using AddConstVolatileLRef = std::add_lvalue_reference_t<std::add_volatile_t<std::add_const_t<T>>>;
		using AddConstVolatileRRef = std::add_rvalue_reference_t<std::add_volatile_t<std::add_const_t<T>>>;
		using AddConstLRefVolatileRRef = std::add_lvalue_reference_t < std::add_volatile_t<std::add_lvalue_reference_t<std::add_const_t<T>>>>;
		using AddConstRRefVolatileLRef = std::add_lvalue_reference_t < std::add_volatile_t<std::add_rvalue_reference_t<std::add_const_t<T>>>>;
		using RemoveConst = std::remove_const_t<T>;
		using RemoveVolatile = std::remove_volatile_t<T>;
		using RemoveRef = std::remove_reference_t<T>;
		using RemovePtr = std::remove_pointer_t<T>;*/

		constexpr bool ea = !is_same_or_v<
			std::add_const_t<int>,
			std::add_lvalue_reference_t<int>,
			std::add_rvalue_reference_t<int>,
			std::add_volatile_t<int>,
			std::add_pointer_t<int>,
			std::add_lvalue_reference_t<std::add_const_t<int>>,
			std::add_rvalue_reference_t<std::add_const_t<int>>,
			std::add_pointer_t<std::add_const_t<int>>,
			std::add_volatile_t<std::add_const_t<int>>
		>;
		static_assert(ea);
		
		/*static_assert(!is_same_or_v<
			std::add_const_t<T>,
			std::add_lvalue_reference_t<T>,
			std::add_volatile_t<T>,
			std::add_pointer_t<T>,
			std::add_lvalue_reference_t<std::add_const_t<T>>,
			std::add_rvalue_reference_t<std::add_const_t<T>>,
			std::add_volatile_t<std::add_const_t<T>>
		>);*/

		static_assert(nox::Check<T, std::add_const_t<T>>());
		static_assert(nox::Check<T, std::add_lvalue_reference_t<T>>());
		static_assert(nox::Check<T, std::add_rvalue_reference_t<T>>());
		static_assert(nox::Check<T, std::add_volatile_t<T>>());
		static_assert(nox::Check<T, std::add_pointer_t<T>>());
		static_assert(nox::Check<T, std::add_lvalue_reference_t<std::add_const_t<T>>>());
		static_assert(nox::Check<T, std::add_rvalue_reference_t<std::add_const_t<T>>>());
		static_assert(nox::Check<T, std::add_volatile_t<std::add_const_t<T>>>());
		static_assert(nox::Check<T, std::add_pointer_t<std::add_const_t<T>>>());
		static_assert(nox::Check<T, std::add_lvalue_reference_t<std::add_pointer_t<T>>>());
		static_assert(nox::Check<T, std::add_rvalue_reference_t<std::add_pointer_t<T>>>());
		static_assert(nox::Check<T, std::add_lvalue_reference_t<std::add_pointer_t<std::add_const_t<T>>>>());
		static_assert(nox::Check<T, std::add_rvalue_reference_t<std::add_pointer_t<std::add_const_t<T>>>>());

		return true;
	}

	//	サンプルケース
	static_assert(CheckGroup<int>() == true);
	static_assert(CheckGroup<int*>() == true);
	static_assert(CheckGroup<int*const>() == true);
	static_assert(CheckGroup<const int*>() == true);
	static_assert(CheckGroup<int&>() == true);
	static_assert(CheckGroup<const int&>() == true);

}

