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

	template<class T>
	struct IComponentTag : public nox::IComponentData
	{
		IComponentTag()noexcept = delete;
	};

	namespace concepts
	{
		template<class T>
		concept Component = std::is_same_v<T, nox::IComponentData> == false && std::is_base_of_v<nox::IComponentData, T>&& std::is_trivially_destructible_v<T>;

		template<class T>
		concept ComponentTag =  std::is_same_v<T, nox::IComponentTag<T>> == false && std::is_base_of_v<nox::IComponentTag<T>, T>&& std::is_trivially_destructible_v<T>;
	}
}