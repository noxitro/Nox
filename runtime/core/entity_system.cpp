// Copyright (C) 2026 NOX ENGINE All rights reserved.

/// @file	entity_system.cpp
/// @brief	entity_system
#include "pch.h"
#include "entity_system.h"

namespace nox::detail
{
	namespace
	{
		/// @brief 登録済みEntitySystem型の連結リスト。記述子自身がノードなのでヒープを使わない。
		const nox::EntitySystemTypeDescriptor* g_entity_system_type_list_head = nullptr;
	}
}

void nox::detail::RegisterEntitySystemType(nox::EntitySystemTypeDescriptor& descriptor)noexcept
{
	//	静的初期化中に呼ばれる。この時点では単一スレッドなので同期は不要。
	const bool already_registered =
		(&descriptor == nox::detail::g_entity_system_type_list_head) || (descriptor.next != nullptr);
	NOX_ASSERT(already_registered == false, u8"EntitySystem型が二重に登録されました");
	if (already_registered)
	{
		return;
	}

	descriptor.next = nox::detail::g_entity_system_type_list_head;
	nox::detail::g_entity_system_type_list_head = &descriptor;
}

const nox::EntitySystemTypeDescriptor* nox::detail::GetEntitySystemTypeListHead()noexcept
{
	return nox::detail::g_entity_system_type_list_head;
}
