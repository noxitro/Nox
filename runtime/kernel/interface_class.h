//	Copyright (C) 2025 NOX ENGINE All rights reserved.

///	@file	interface_class.h
///	@brief	インターフェースの基底クラス
#pragma once
#include	<type_traits>

namespace nox
{
	struct Interface
	{
		inline constexpr Interface()noexcept = default;
	};

	template<class T>
	constexpr bool IsInterfaceValue = std::is_base_of_v<nox::Interface, T> && std::is_polymorphic_v<T> == false && sizeof(T) <= 1U;
}