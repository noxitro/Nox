///	@file	advanced_definition.h
///	@brief	advanced_definition
#pragma once
#include	"basic_definition.h"
#include	"assertion.h"
#include	"string_format.h"
#include	"../type_traits/type_name.h"

namespace nox::util
{
	/// @brief	ポインタの参照先を取得する　nullptrだった場合例外を投げる
	/// @tparam T 
	/// @param ptr 
	/// @return 
	template<class T> requires(std::is_pointer_v<T>)
		inline constexpr std::remove_pointer_t<T>& Deref(const T& ptr)
	{
		NOX_ASSERT(ptr != nullptr, util::Format(u"{0}はnullptrです", util::GetTypeName<T>()));
		return *ptr;
	}

	template<class T> requires(std::is_pointer_v<T> && std::is_pointer_v<std::remove_pointer_t<T>>)
	inline constexpr std::remove_pointer_t<T> SafeDeref(const T& ptr)noexcept
	{
		if (ptr == nullptr)
		{
			return nullptr;
		}
		return *ptr;
	}
}