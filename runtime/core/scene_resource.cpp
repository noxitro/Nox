//	Copyright (c) 2025 NOX ENGINE All rights reserved.

///	@file	scene_resource.cpp
///	@brief	scene_resource
#include	"pch.h"
#include	"scene_resource.h"

#include	"entity.h"
#include	"world.h"
#include	"log_id.h"

namespace nox
{
	namespace
	{
		struct ComponentData
		{
			nox::StlU8String fqn;

		};

		struct SceneNode
		{
			nox::Guid id;
			nox::Vector<ComponentData> component_list;
		};
	}

	struct SceneResource::Data
	{
		nox::Vector<SceneNode> node_list;
	};
}

nox::SceneResource::SceneResource()noexcept :
	data_(nullptr)
{
}

void nox::SceneResource::Instantiate(nox::World& world)const
{
	NOX_ASSERT(data_ != nullptr, u8"invalid scene resource data.");
	for (const SceneNode& node : data_->node_list)
	{
		nox::EntityId entity = world.CreateEntity();
		for (const ComponentData& component_data : node.component_list)
		{
			const auto type = nox::reflection::FindClassInfo(component_data.fqn);
			if (type == nullptr)
			{
				NOX_ERROR_LINE(nox::log_id::Resource, u8"failed to find component type: {0}", component_data.fqn);
				continue;
			}
			nox::IComponentData* component = world.CreateComponent(entity, type->GetType());
			if (component == nullptr)
			{
				NOX_ERROR_LINE(nox::log_id::Resource, u8"failed to create component: {0}", component_data.fqn);
				continue;
			}
		}
	}
}

bool nox::SceneResource::OnInitialize(nox::io::BinaryReader& reader)
{

	return true;
}