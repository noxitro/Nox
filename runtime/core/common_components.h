// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

/// @file	common_components.h
/// @brief	common_components
#pragma once
#include	"component.h"

namespace nox
{
	class NameComponent : public nox::Component
	{
		NOX_DECLARE_OBJECT(NameComponent, nox::Component);
	public:
		inline std::u8string_view GetName()const noexcept { return name_; }
		inline void SetName(std::u8string_view name) { name_ = name; }

	private:
		nox::U8String name_;
	};
	static_assert(nox::IsComponentType<NameComponent>());

	struct EntityGuid : nox::IComponentData
	{
		nox::Guid guid;
	};
	static_assert(nox::IsComponentDataType<EntityGuid>());

	/// @brief Entityにタグを付与
	struct EntityTag : nox::IComponentData
	{
		nox::uint64 tag;
	};
}