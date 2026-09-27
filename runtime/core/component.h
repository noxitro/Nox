//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	component.h
///	@brief	component
#pragma once

#include	"object.h"

namespace nox
{
	/// @brief componentタグ
	struct IComponentData
	{

	};

	class Component : public nox::Object, IComponentData
	{
		NOX_DECLARE_OBJECT(Component, nox::Object);
	};

	template<class T>
	inline consteval bool IsComponentDataType()noexcept
	{
		return 
			std::is_base_of_v<nox::IComponentData, T> &&
			std::is_abstract_v<T> == false &&
			std::is_trivially_copyable_v<T> &&
			std::is_default_constructible_v<T>
			;
	}

	template<class T>
	inline consteval bool IsComponentType()noexcept
	{
		return
			std::is_base_of_v<nox::Component, T> &&
			std::is_trivially_copyable_v<T> == false
			;
	}
}