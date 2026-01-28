//	Copyright (C) 2025 NOX ENGINE All rights reserved.

///	@file	algorithm_lite.h
///	@brief	依存関係が極力ないアルゴリズム系ユーティリティ
#pragma once
#include	"type_traits/type_traits.h"
#include	"type_traits/concepts.h"

namespace nox::util
{
	namespace detail
	{
		template<class T> requires(sizeof(T) >= 0)
			inline constexpr std::size_t SafeSizeofImpl()noexcept
		{
			return sizeof(T);
		}

		template<class T>
		inline constexpr std::size_t SafeSizeofImpl()noexcept
		{
			return 0;
		}
	}

	template<class T>
	inline constexpr std::size_t SafeSizeof()noexcept
	{
		return nox::util::detail::SafeSizeofImpl<T>();
	}
}