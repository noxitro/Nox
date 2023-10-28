///	@file	algorithm.h
///	@brief	アルゴリズム群
#pragma once

#include	<xutility>
#include	<functional>
#include	"../type_traits/type_traits.h"
#include	"../type_traits/concepts.h"
#include	"../type_traits/function_signature.h"

/// @brief ユーティリティ
namespace nox::util
{
#pragma region STL拡張
	template<std::ranges::range T, class FuncType>
		requires(std::is_invocable_r_v<bool, FuncType, typename T::value_type>)
	inline std::add_pointer_t<typename T::value_type> FindIf(const T& range, const FuncType func) noexcept(IsFunctionNoexceptValue<FuncType>)
	{
		decltype(auto) result = std::ranges::find_if(range, func);
		if (result == std::end(range))
		{
			return nullptr;
		}
		else
		{
			return &*result;
		}
	}

	/// @brief 範囲の中から、指定された条件を満たす最初の要素を検索する
	/// @tparam FuncType 関数の型
	/// @tparam T 範囲の型
	/// @param begin 範囲の先頭を指すイテレータ
	/// @param last 範囲の末尾の次を指すイテレータ
	/// @param func 比較関数
	/// @return 見つかった場合はnullptrを返す
	template<nox::concepts::Pointer T, class FuncType> requires(std::is_invocable_r_v<bool, FuncType, std::remove_pointer_t<T>>)
		inline constexpr T FindIf(const T begin, const T last, const FuncType func) noexcept(IsFunctionNoexceptValue<FuncType>)

	{
		decltype(auto) result = std::ranges::find_if(begin, last, func);
		if (result == last)
		{
			return nullptr;
		}
		else
		{
			return &*result;
		}
	}
#pragma endregion


	/// @brief 要素数の範囲外チェック
	/// @tparam T 型
	/// @param container コンテナ
	/// @param index 要素インデックス
	/// @return 範囲外かどうか
	template<class T> requires(std::is_array_v<T> || std::is_invocable_v<decltype(&T::size), T>)
		[[nodiscard]]	inline	constexpr	bool	IsValidIndex(const T& container, const size_t index)noexcept
	{
		return index < std::size(container);
	}

	/// @brief 列挙型Tの値を基底型に変換する
	/// @tparam T 列挙型
	/// @param value 列挙型の値
	/// @return 基底型の値
	template<typename T>
	[[nodiscard]] inline	constexpr	std::underlying_type_t<T> ToUnderlying(const T value)noexcept
	{
#if defined(__clang__)
		return static_cast<std::underlying_type_t<T>>(value);
#else
		return std::to_underlying(value);
#endif
	}

#pragma region bit操作
	template<concepts::Enum T>
	inline	constexpr T BitOr(const std::initializer_list<T> lst)noexcept
	{
		std::underlying_type_t<T> ret = static_cast<std::underlying_type_t<T>>(0.0); //ValueTraits<std::underlying_type_t<T>>::VALUE0();
		for (typename std::initializer_list<T>::iterator it = lst.begin(); it != lst.end(); ++it)
		{
			ret |= util::ToUnderlying(*it);
		}
		return static_cast<T>(ret);
	}

	template<concepts::ClassUnion T>
	[[nodiscard]] inline constexpr T BitOr(const T& a, const T& b)noexcept { return std::bit_or<T>()(a, b); }

	template<concepts::Enum T>
	[[nodiscard]] inline constexpr T BitOr(const T a, const T b)noexcept { return static_cast<T>(std::bit_or<std::underlying_type_t<T>>()(ToUnderlying(a), ToUnderlying(b))); }

	template<concepts::Arithmetic T>
	[[nodiscard]] inline constexpr T BitOr(const T a, const T b)noexcept { return std::bit_or<T>()(a, b); }


	template<concepts::ClassUnion T>
	inline	constexpr	bool IsBitAnd(const T& a, const T& b)noexcept { return std::bit_and<T>()(a, b); }

	template<concepts::Enum T>
	inline	constexpr	bool IsBitAnd(const T a, const T b)noexcept { return std::bit_and<std::underlying_type_t<T>>()(ToUnderlying(a), ToUnderlying(b)); }

	template<concepts::Arithmetic T>
	inline	constexpr	bool IsBitAnd(const T a, const T b)noexcept { return std::bit_and<T>()(a, b); }

	template<concepts::ClassUnion T>
	inline	constexpr	T BitAnd(const T& a, const T& b)noexcept { return a & b; }

	template<concepts::Enum T>
	inline	constexpr	T BitAnd(const T a, const T b)noexcept { return static_cast<T>(ToUnderlying(a) & ToUnderlying(b)); }

	template<concepts::Arithmetic T>
	inline	constexpr	T BitAnd(const T a, const T b)noexcept { return a & b; }

	template<concepts::ClassUnion T>
	[[nodiscard]] inline constexpr T BitXor(const T& a, const T& b)noexcept { return std::bit_xor<T>()(a, b); }

	template<concepts::Arithmetic T>
	[[nodiscard]] inline constexpr T BitXor(const T a, const T b)noexcept { return std::bit_xor<T>()(a, b); }

	template<concepts::Enum T>
	[[nodiscard]] inline constexpr T BitXor(const T a, const T b)noexcept { return static_cast<T>(BitXor(ToUnderlying(a), ToUnderlying(b))); }
#pragma endregion

	
	/// @brief delete呼び出し後nullptrを格納する
	/// @tparam T ポインタの型
	/// @param ptr delete対象のポインタ
	template<concepts::Pointer T> 
	inline constexpr void SafeDelete(T& ptr)
	{
		if (ptr != nullptr)
		{
			delete ptr;
			ptr = nullptr;
		}
	}
}