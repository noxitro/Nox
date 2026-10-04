//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	component_id.cpp
///	@brief	component_id
#include	"pch.h"
#include	"component_id.h"

namespace nox::detail
{
	namespace
	{
		/// @brief Type&のハッシュから ComponentTypeIndex を引くテーブル
		constinit std::array<nox::uint16, nox::detail::kMaxComponentTypeCount> component_type_indices_{};

		constinit nox::Atomic<nox::uint16> g_component_count{ 0u };

	}
}

nox::uint16 nox::detail::GetComponentId(const nox::reflection::Type& type)noexcept
{
	static constinit nox::Atomic<nox::uint16> next_id{ 0u };
	const nox::uint16 id = next_id.fetch_add(1u, std::memory_order_relaxed);
	NOX_ASSERT(id < nox::detail::kMaxComponentTypeCount, u8"ComponentData型の登録数が上限({0})を超えました", nox::detail::kMaxComponentTypeCount);
	return id;
}