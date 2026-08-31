// Copyright (C) 2026 NOX ENGINE All rights reserved.

/// @file	component_type.cpp
/// @brief	component_type
#include "pch.h"
#include "component_type.h"

namespace nox::detail
{
	namespace
	{
		/// @brief 登録済みComponentTypeInfoの表。静的領域のみでヒープを使わない。
		std::array<std::atomic<const nox::ComponentTypeInfo*>, nox::k_max_component_type_count> g_component_type_table{};
		std::atomic<nox::uint32> g_next_component_type_index{ 0u };
	}
}

nox::ComponentTypeIndex nox::detail::AcquireComponentTypeIndex()noexcept
{
	const nox::uint32 index = nox::detail::g_next_component_type_index.fetch_add(1u, std::memory_order_relaxed);
	NOX_ASSERT(index < nox::k_max_component_type_count, u8"ComponentData型の登録数が上限({0})を超えました", nox::k_max_component_type_count);
	if (index >= nox::k_max_component_type_count)
	{
		std::abort();
	}
	return static_cast<nox::ComponentTypeIndex>(index);
}

const nox::ComponentTypeInfo* nox::detail::TryGetComponentTypeInfo(const nox::ComponentTypeIndex index)noexcept
{
	if (index >= nox::k_max_component_type_count)
	{
		return nullptr;
	}
	return nox::detail::g_component_type_table[index].load(std::memory_order_acquire);
}

void nox::detail::RegisterComponentTypeInfo(const nox::ComponentTypeInfo& info)noexcept
{
	NOX_ASSERT(info.index < nox::k_max_component_type_count, u8"ComponentTypeIndexが範囲外です");
	if (info.index >= nox::k_max_component_type_count)
	{
		return;
	}
	nox::detail::g_component_type_table[info.index].store(&info, std::memory_order_release);
}
