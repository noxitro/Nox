// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

/// @file	sorted_tuple.h
/// @brief	コンパイル時にタプルの要素型を比較器で並べ替える SortedTuple
#pragma once
#include	<tuple>
#include	<type_traits>

#include	"type_name.h"

namespace nox
{
	namespace detail
	{
		/// @brief 型のみを保持する軽量な型リスト
		template<class... Ts>
		struct TypeList {};

		/// @brief 比較器を型ペア (T, U) に対して評価する
		/// @tparam Compare operator()(const T&, const U&) を持つ比較器
		/// @note 比較器は引数の値を参照しない前提。T / U は constexpr 既定構築可能である必要があります。
		template<class Compare, class T, class U>
		[[nodiscard]] constexpr bool CompareTypeLess()noexcept
		{
			return Compare{}(T{}, U{});
		}

		/// @brief 型リストの先頭へ型を追加
		template<class T, class List>
		struct PushFront;

		template<class T, class... Ts>
		struct PushFront<T, TypeList<Ts...>>
		{
			using type = TypeList<T, Ts...>;
		};

		/// @brief ソート済み型リストへ 1 要素を安定挿入する
		template<class Compare, class T, class List>
		struct InsertSorted;

		template<class Compare, class T>
		struct InsertSorted<Compare, T, TypeList<>>
		{
			using type = TypeList<T>;
		};

		template<class Compare, class T, class Head, class... Tail>
		struct InsertSorted<Compare, T, TypeList<Head, Tail...>>
		{
			//	T は Head より前方由来のため、Head < T が偽（= T <= Head）なら T を前に置く。
			//	これにより同値要素の相対順序が保たれる（安定ソート）。
			using type = std::conditional_t<
				!CompareTypeLess<Compare, Head, T>(),
				TypeList<T, Head, Tail...>,
				typename PushFront<Head, typename InsertSorted<Compare, T, TypeList<Tail...>>::type>::type
			>;
		};

		/// @brief 型リストを比較器で安定挿入ソートする
		template<class Compare, class List>
		struct SortTypeList;

		template<class Compare>
		struct SortTypeList<Compare, TypeList<>>
		{
			using type = TypeList<>;
		};

		template<class Compare, class Head, class... Tail>
		struct SortTypeList<Compare, TypeList<Head, Tail...>>
		{
			using type = typename InsertSorted<Compare, Head, typename SortTypeList<Compare, TypeList<Tail...>>::type>::type;
		};

		/// @brief タプル型テンプレートから型リストを取り出し、同じテンプレートで再構築する
		template<class TupleType>
		struct TupleTypeListTraits;

		template<template<class...> class Tpl, class... Ts>
		struct TupleTypeListTraits<Tpl<Ts...>>
		{
			using list = TypeList<Ts...>;

			template<class... Us>
			using rebind = Tpl<Us...>;
		};

		/// @brief ソート済み型リストを元のタプルテンプレートへ適用する
		template<class TupleType, class List>
		struct RebuildTuple;

		template<class TupleType, class... Ts>
		struct RebuildTuple<TupleType, TypeList<Ts...>>
		{
			using type = typename TupleTypeListTraits<TupleType>::template rebind<Ts...>;
		};
	}

	/// @brief 要素型を Compare で安定ソートしたタプル型を表すエイリアス
	/// @tparam TupleType std::tuple などの型テンプレート
	/// @tparam Compare   operator()(const T&, const U&) を持つ比較器（値は参照しない前提）
	/// @note 各要素型は constexpr 既定構築可能（リテラル型）である必要があります。
	/// @code
	///   using T = nox::SortedTuple<std::tuple<int, float, double>, nox::SignatureLess>;
	///   // T == std::tuple<double, float, int>
	/// @endcode
	template<class TupleType, class Compare>
	using SortedTuple = typename detail::RebuildTuple<
		TupleType,
		typename detail::SortTypeList<Compare, typename detail::TupleTypeListTraits<TupleType>::list>::type
	>::type;

	/// @brief 型名（シグネチャ）の辞書順で並べ替える比較器
	struct SignatureLess
	{
		template<class T, class U>
		static constexpr bool operator()(const T&, const U&)noexcept
		{
			return nox::util::GetTypeName<T>() < nox::util::GetTypeName<U>();
		}
	};
}
