//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	component_id.h
///	@brief	component_id
#pragma once
#include	"component.h"

namespace nox
{
	enum class ComponentId : nox::uint16 {};
}

namespace nox::detail
{
	constexpr nox::uint16 kMaxComponentTypeCount = std::numeric_limits<nox::uint16>::max();

	const nox::reflection::Type& GetComponentType(const nox::ComponentId id)noexcept;
	nox::ComponentId GetComponentId(const nox::reflection::Type& type)noexcept;
	nox::ComponentId AssignComponentId(const nox::reflection::Type& type)noexcept;

	template<std::derived_from<nox::detail::IComponentData> T>
	inline nox::ComponentId GetComponentId()noexcept
	{
		static const nox::ComponentId id = nox::detail::GetComponentId(nox::reflection::Typeof<T>());
		return id;
	}
}