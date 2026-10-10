//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	component_id.h
///	@brief	component_id
#pragma once
#include	"component.h"

namespace nox::detail
{
	constexpr nox::uint16 kMaxComponentTypeCount = std::numeric_limits<nox::uint16>::max();

	const nox::reflection::Type& GetComponentType(const nox::uint16 id)noexcept;
	nox::uint16 GetComponentId(const nox::reflection::Type& type)noexcept;
	nox::uint16 AssignComponentId(const nox::reflection::Type& type)noexcept;

	template<std::derived_from<nox::detail::IComponentData> T>
	inline nox::uint16 GetComponentId()noexcept
	{
		static const nox::uint16 id = nox::detail::GetComponentId(nox::reflection::Typeof<T>());
		return id;
	}
}