//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	component.h
///	@brief	component
#pragma once
#include	"ecs_definitions.h"

namespace nox
{
	/// @brief componentタグ
	struct IComponentData
	{

	};

	namespace concepts
	{
		template<class T>
		concept Component = std::is_same_v<T, nox::IComponentData> == false && std::is_base_of_v<nox::IComponentData, T>&& std::is_trivially_destructible_v<T>;
	}

	template<class T>
	struct Component : public nox::IComponentData
	{
	protected:
		static consteval bool StaticDeclareVerify()noexcept
		{
			return true;
		}
	};
}