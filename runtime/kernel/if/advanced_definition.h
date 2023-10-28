///	@file	advanced_definition.h
///	@brief	advanced_definition
#pragma once
#include	"basic_definition.h"
#include	"assertion.h"

namespace nox::util
{
	template<class T> requires(std::is_pointer_v<T>)
		inline constexpr std::remove_pointer_t<T>& Deref(const T& ptr)
	{
		NOX_CONDITINAL_DEBUG(debug::Assert(ptr != nullptr, u"null ptr"));
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