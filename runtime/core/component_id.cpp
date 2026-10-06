//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	component_id.cpp
///	@brief	component_id
#include	"pch.h"
#include	"component_id.h"
#include	"world.h"

namespace nox::detail
{
	namespace
	{
		struct
		{
			nox::UnorderedMap<const nox::reflection::Type*, nox::uint16> type_to_id_map_;
		}* cache_ = nullptr;

		constinit std::array<std::byte, sizeof(std::remove_pointer_t<decltype(cache_)>)> cache_storage{ std::byte{} };
		constinit nox::Atomic<nox::uint16> g_component_count{ 0u };
		constinit nox::ReadWriteLock g_component_type_indices_lock{};

		constinit std::array<const nox::reflection::Type*, nox::detail::kMaxComponentTypeCount> component_type_indices_{ nullptr };
	}
}

void nox::World::GlobalInitialize()
{
	nox::detail::cache_ = std::construct_at(reinterpret_cast<std::remove_pointer_t<decltype(nox::detail::cache_)>*>(nox::detail::cache_storage.data()));
}

void nox::World::GlobalTerminate()
{
	std::destroy_at(reinterpret_cast<std::remove_pointer_t<decltype(nox::detail::cache_)>*>(nox::detail::cache_storage.data()));
	nox::detail::cache_ = nullptr;
}

const nox::reflection::Type& nox::detail::GetComponentType(const nox::uint16 id)noexcept
{
	//	idでアクセスする辞典でキャッシュには登録済みのはずなので、ロックなしでアクセスする
	return *nox::detail::component_type_indices_[id];
}

nox::uint16 nox::detail::GetComponentId(const nox::reflection::Type& type)noexcept
{
	{
		NOX_LOCAL_SCOPE(nox::ScopedReadLock(g_component_type_indices_lock));
		if (const auto it = nox::detail::cache_->type_to_id_map_.find(&type); it != nox::detail::cache_->type_to_id_map_.end())
		{
			return it->second;
		}
	}

	return nox::detail::AssignComponentId(type);
}

nox::uint16 nox::detail::AssignComponentId(const nox::reflection::Type& type)noexcept
{
	const nox::uint16 id = g_component_count.fetch_add(1u, std::memory_order_relaxed);

	{
		NOX_LOCAL_SCOPE(nox::ScopedWriteLock(g_component_type_indices_lock));
		nox::detail::cache_->type_to_id_map_.emplace(&type, id);
		nox::detail::component_type_indices_[id] = &type;
	}

	return id;
}