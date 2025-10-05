//	Copyright (c) 2025 NOX ENGINE All rights reserved.

///	@file	behavior_manager.cpp
///	@brief	behavior_manager
#include	"stdafx.h"
#include	"behavior_manager.h"

#include	"behavior.h"
#include	"update_order_attribute.h"
#include	"log_id.h"

void nox::BehaviorManager::Initialize()
{
}

void nox::BehaviorManager::Update()
{
	for (const auto& group : behavior_group_list_)
	{
		for (nox::Behavior& behavior : group.behavior_list)
		{
			behavior.Update();
		}
	}
}

void nox::BehaviorManager::LateUpdate()
{
	for (const auto& group : behavior_group_list_)
	{
		for (nox::Behavior& behavior : group.behavior_list)
		{
			behavior.LateUpdate();
		}
	}
}

void nox::BehaviorManager::Finalize()
{

}

void nox::BehaviorManager::Register(nox::Behavior& behavior)
{
	nox::int32 priority = 0;
	{
		nox::attr::UpdateOrder* update_order = nox::reflection::AsCast<nox::attr::UpdateOrder>(behavior);
		if (update_order != nullptr)
		{
			priority = update_order->GetPriority();
		}
	}

	{
		NOX_LOCAL_SCOPE(nox::os::ScopedLock{ mutex_ });

		decltype(auto) it = std::ranges::find_if(behavior_group_list_, [priority](const BehaviorGroup& group) {
			return group.update_order == priority;
			});

		if (it == behavior_group_list_.end())
		{
			it = behavior_group_list_.insert(it, BehaviorGroup{ priority });
		}

		it->behavior_list.emplace_back(behavior);
	}
}

void nox::BehaviorManager::Unregister(nox::Behavior& behavior)
{
	NOX_LOCAL_SCOPE(nox::os::ScopedLock{ mutex_ });
	for (BehaviorGroup& group : behavior_group_list_)
	{
		const auto it = std::ranges::find_if(group.behavior_list, [&behavior](const std::reference_wrapper<nox::Behavior>& ref) {
			return &ref.get() == &behavior;
			});
		if (it != group.behavior_list.end())
		{
			group.behavior_list.erase(it);
			break;
		}
	}
}