//	Copyright (c) 2025 NOX ENGINE All rights reserved.

#include	"pch.h"
#if NOX_DEVELOP
#include	"remote_system.g.h"
#include	"../../world.h"
#include	"../../attribute_dev_common.h"
#include	"../../component.h"
#include	"../../scene_manager.h"
#include	"../../scene_view.h"
#include	"../editor_remote_server.h"
#include	"../socket_stream_utility.h"
#include	"../../asset_manager.h"

namespace
{
	const char* ToAscii(nox::memory::InstanceType type) noexcept
	{
		switch (type)
		{
		case nox::memory::InstanceType::Object: return "Object";
		case nox::memory::InstanceType::Stl: return "Stl";
		case nox::memory::InstanceType::Other: return "Other";
		default: return "Unknown";
		}
	}

	const char* ToAscii(nox::memory::SegmentType type) noexcept
	{
		switch (type)
		{
		case nox::memory::SegmentType::Default: return "Default";
		case nox::memory::SegmentType::Boot: return "Boot";
		case nox::memory::SegmentType::Resource: return "Resource";
		case nox::memory::SegmentType::Render: return "Render";
		case nox::memory::SegmentType::Develop: return "Develop";
		default: return "Unknown";
		}
	}

	void AppendAscii(std::u8string& text, const char* line, int length)
	{
		if (length <= 0)
		{
			return;
		}
		text.append(reinterpret_cast<const char8_t*>(line), static_cast<std::size_t>(length));
	}
}

nox::PlacementObject<nox::dev::editor_remote::Response> nox::dev::editor_remote::AssetConvertQuery::Execute(nox::World& world, std::span<nox::uint8> storage)const
{
	return nullptr;
}

nox::PlacementObject<nox::dev::editor_remote::Response> nox::dev::editor_remote::GetMainSceneView::Execute(nox::World& world, std::span<nox::uint8> storage)const
{
	auto scene_view_info = nox::PlacementObject<nox::dev::editor_remote::SceneViewInfo>::Construct(storage);
	nox::SceneManager& scene_manager = world.GetSystem<nox::SceneManager>();
	auto& scene_view = scene_manager.GetMainSceneView();
	auto window_handle = scene_view.GetWindow().GetNativeHandle();

	scene_view_info->SetMainWindowHandle(reinterpret_cast<nox::intptr>(window_handle));
	return scene_view_info;
}

nox::PlacementObject<nox::dev::editor_remote::Response> nox::dev::editor_remote::SyncQuery::Execute(nox::World& world, std::span<nox::uint8> storage)const
{
	nox::dev::editor_remote::EditorRemoteServer& server = world.GetSystem<nox::dev::editor_remote::EditorRemoteServerSystem>().GetServer();
	const nox::int64 remote_instance_id = GetRemoteInstanceId();
	NOX_ASSERT(remote_instance_id > 0, u"Editor owned remote instance id must be positive. id:{0}", remote_instance_id);
	auto response = nox::PlacementObject<nox::dev::editor_remote::SyncResponse>::Construct(storage);
	response->SetRemoteInstanceId(remote_instance_id);

	if (nox::Object* const registered_object = server.FindRemoteInstance(remote_instance_id))
	{
		nox::Object* const managed_object = static_cast<nox::Object*>(registered_object);
		NOX_ASSERT(managed_object != nullptr, u"Registered remote instance is not ManagedObject. id:{0}", remote_instance_id);
		if (managed_object != nullptr)
		{
			nox::dev::editor_remote::SetPropertiesFromBytes(GetPropertyByteBuffer(), *managed_object);
			response->SetApplied(true);
		}
		return response;
	}

	const nox::reflection::ClassInfo* const class_info = nox::reflection::FindClassInfo(GetFqn());
	NOX_ASSERT(class_info != nullptr, u"SyncQuery unknown type: {0}", GetFqn());
	if (class_info == nullptr)
	{
		response->SetApplied(false);
		return response;
	}

	nox::IntrusivePtr<nox::Object> object;
	//if (class_info->GetType() == nox::reflection::Typeof<nox::EntityNode>())
	//{
	//	nox::IntrusivePtr<nox::EntityNode> entity_node = nox::EntityNode::Create(u8"");
	//	object = std::move(entity_node);
	//}
	//else
	//{
	//	NOX_ASSERT(class_info->IsSubclassOf<nox::Object>(), u"SyncQuery type must inherit Object: {0}", GetFqn());
	//	nox::Object* const created_object = static_cast<nox::Object*>(class_info->GetType().CreateObject());
	//	nox::Object* const managed_object = nox::reflection::AsCast<nox::Object*>(created_object);
	//	NOX_ASSERT(managed_object != nullptr, u"SyncQuery instance creation failed: {0}", GetFqn());
	//	object.Reset(managed_object);
	//}

	if (object == nullptr)
	{
		response->SetApplied(false);
		return response;
	}

	/*if (nox::Node* const node = nox::reflection::AsCast<nox::Node*>(object.Get()))
	{
		node->SetApplication(application);
	}*/

	nox::dev::editor_remote::SetPropertiesFromBytes(GetPropertyByteBuffer(), *object.Get());
	server.RegisterEditorOwnedRemoteInstance(*object.Get(), remote_instance_id);
	response->SetApplied(true);
	return response;
}

nox::PlacementObject<nox::dev::editor_remote::Response> nox::dev::editor_remote::AddEntityNodeQuery::Execute(nox::World& world, std::span<nox::uint8> storage)const
{
	//nox::dev::editor_remote::EditorRemoteServer& server = world.GetSystem<nox::dev::editor_remote::EditorRemoteServerSystem>().GetServer();
	//nox::SceneNode& main_scene = world.GetSystem<nox::SceneManager>().GetMainScene();
	//const nox::int64 remote_instance_id = GetRemoteInstanceId();
	//NOX_ASSERT(remote_instance_id > 0, u"Editor owned EntityNode id must be positive. id:{0}", remote_instance_id);
	//auto response = nox::PlacementObject<nox::dev::editor_remote::AddEntityNodeResponse>::Construct(storage);
	//response->SetRemoteInstanceId(remote_instance_id);

	//nox::Object* registered_object = server.FindRemoteInstance(remote_instance_id);
	//nox::EntityNode* entity_node = registered_object != nullptr ? static_cast<nox::EntityNode*>(registered_object) : nullptr;
	//if (entity_node == nullptr)
	//{
	//	nox::IntrusivePtr<nox::EntityNode> created_entity_node = nox::EntityNode::Create(GetName());
	//	entity_node = created_entity_node.Get();
	//	entity_node->SetApplication(application);
	//	server.RegisterEditorOwnedRemoteInstance(*entity_node, remote_instance_id);
	//	response->SetCreated(true);
	//}
	//else
	//{
	//	entity_node->SetName(GetName());
	//}

	//nox::Node* parent = &main_scene;
	//const nox::int64 parent_remote_instance_id = GetParentRemoteInstanceId();
	//if (parent_remote_instance_id != 0)
	//{
	//	nox::Object* const parent_object = server.FindRemoteInstance(parent_remote_instance_id);
	//	parent = parent_object != nullptr ? static_cast<nox::Node*>(parent_object) : nullptr;
	//	NOX_ASSERT(parent != nullptr, u"Parent EntityNode was not registered. id:{0}", parent_remote_instance_id);
	//	if (parent == nullptr)
	//	{
	//		parent = &main_scene;
	//	}
	//}

	//main_scene.AddEntity(*entity_node, parent);
	//response->SetAttached(true);
	//return response;
	return {};
}

nox::PlacementObject<nox::dev::editor_remote::Response> nox::dev::editor_remote::AddComponentQuery::Execute(nox::World& world, std::span<nox::uint8> storage)const
{
	/*nox::dev::editor_remote::EditorRemoteServer& server = world.GetSystem<nox::dev::editor_remote::EditorRemoteServerSystem>().GetServer();
	const nox::int64 remote_instance_id = GetRemoteInstanceId();
	NOX_ASSERT(remote_instance_id > 0, u"Editor owned Component id must be positive. id:{0}", remote_instance_id);
	auto response = nox::PlacementObject<nox::dev::editor_remote::AddComponentResponse>::Construct(storage);
	response->SetRemoteInstanceId(remote_instance_id);

	nox::Object* const entity_object = server.FindRemoteInstance(GetEntityNodeRemoteInstanceId());
	nox::EntityNode* const entity_node = entity_object != nullptr ? static_cast<nox::EntityNode*>(entity_object) : nullptr;
	NOX_ASSERT(entity_node != nullptr, u"AddComponentQuery target EntityNode was not registered. id:{0}", GetEntityNodeRemoteInstanceId());
	if (entity_node == nullptr)
	{
		return response;
	}

	nox::Object* registered_object = server.FindRemoteInstance(remote_instance_id);
	nox::Component* component = registered_object != nullptr ? nox::reflection::AsCast<nox::Component*>(registered_object) : nullptr;
	if (component == nullptr)
	{
		const nox::reflection::ClassInfo* const class_info = nox::reflection::FindClassInfo(GetComponentTypeFqn());
		NOX_ASSERT(class_info != nullptr, u"AddComponentQuery unknown component type: {0}", GetComponentTypeFqn());
		if (class_info == nullptr || class_info->IsSubclassOf<nox::Component>() == false)
		{
			return response;
		}

		if (class_info->GetType() == nox::reflection::Typeof<nox::Transform>())
		{
			component = entity_node->GetSameComponent(nox::reflection::Typeof<nox::Transform>());
			if (component == nullptr)
			{
				component = entity_node->CreateComponent<nox::Transform>();
			}
		}
		else
		{
			component = entity_node->CreateComponent(class_info->GetType());
		}
		if (component == nullptr)
		{
			return response;
		}

		server.RegisterRemoteInstance(*component, remote_instance_id);
		response->SetCreated(true);
	}

	nox::dev::editor_remote::SetPropertiesFromBytes(GetPropertyByteBuffer(), *component);
	response->SetAdded(component->IsValid());
	return response;*/
	return {};
}

nox::PlacementObject<nox::dev::editor_remote::Response> nox::dev::editor_remote::DestroyEntityNodeQuery::Execute(nox::World& world, std::span<nox::uint8>)const
{
	/*nox::dev::editor_remote::EditorRemoteServer& server = world.GetSystem<nox::dev::editor_remote::EditorRemoteServerSystem>().GetServer();
	nox::Object* const object = server.FindRemoteInstance(GetRemoteInstanceId());
	if (object == nullptr)
	{
		return nullptr;
	}

	nox::EntityNode* const entity_node = static_cast<nox::EntityNode*>(object);
	NOX_ASSERT(entity_node != nullptr, u"DestroyEntityNodeQuery target is not EntityNode. id:{0}", GetRemoteInstanceId());
	if (entity_node == nullptr)
	{
		return nullptr;
	}

	UnregisterEntityComponents(server, *entity_node);
	world.GetSystem<nox::SceneManager>().GetMainScene().RemoveEntity(*entity_node);
	server.UnregisterRemoteInstance(GetRemoteInstanceId());
	return nullptr;*/
	return {};
}

nox::PlacementObject<nox::dev::editor_remote::Response> nox::dev::editor_remote::AutoSyncQuery::Execute(nox::World& world, std::span<nox::uint8> storage)const
{
	nox::dev::editor_remote::EditorRemoteServer& server = world.GetSystem<nox::dev::editor_remote::EditorRemoteServerSystem>().GetServer();
	auto response = nox::PlacementObject<nox::dev::editor_remote::AutoSyncResponse>::Construct(storage);
	response->SetRemoteInstanceId(GetRemoteInstanceId());

	nox::Object* const object = server.FindRemoteInstance(GetRemoteInstanceId());
	nox::Object* const managed_object = object != nullptr ? static_cast<nox::Object*>(object) : nullptr;
	if (managed_object == nullptr)
	{
		response->SetExists(false);
		return response;
	}

	std::array<nox::uint8, 2048> property_byte_buffer{};
	(void)nox::dev::editor_remote::GetPropertiesBytes(property_byte_buffer, *managed_object);
	response->SetExists(true);
	response->SetPropertyByteBuffer(property_byte_buffer);
	return response;
}

nox::PlacementObject<nox::dev::editor_remote::Response> nox::dev::editor_remote::InvokeRuntimeActionQuery::Execute(nox::World& world, std::span<nox::uint8> storage)const
{
	nox::dev::editor_remote::EditorRemoteServer& server = world.GetSystem<nox::dev::editor_remote::EditorRemoteServerSystem>().GetServer();
	auto response = nox::PlacementObject<nox::dev::editor_remote::InvokeRuntimeActionResponse>::Construct(storage);
	response->SetRemoteInstanceId(GetRemoteInstanceId());

	nox::Object* const object = server.FindRemoteInstance(GetRemoteInstanceId());
	if (object == nullptr)
	{
		response->SetInvoked(false);
		return response;
	}

	const nox::reflection::ClassInfo* const class_info = nox::reflection::FindClassInfo(object->GetType());
	NOX_ASSERT(class_info != nullptr, u8"Action invoke target type is not registered. id:{0}", GetRemoteInstanceId());
	if (class_info == nullptr)
	{
		response->SetInvoked(false);
		return response;
	}

	for (const nox::reflection::FunctionInfo& function_info : class_info->GetFunctionList())
	{
		if (function_info.GetFullName() != GetFunctionFullName())
		{
			continue;
		}
		if (function_info.GetAttribute<nox::attr::dev::Action>() == nullptr ||
			function_info.GetNonDefaultParamLength() != 0 ||
			function_info.IsNoReturn() == false)
		{
			NOX_ASSERT(false, u8"Invalid inspector action function: {0}", GetFunctionFullName());
			response->SetInvoked(false);
			return response;
		}

		std::optional<std::monostate> invoke_result;
		if (function_info.IsStatic())
		{
			invoke_result = static_cast<const nox::reflection::detail::FunctionInfoImpl<void>&>(function_info).InvokeImpl(std::span<void*>{});
		}
		else
		{
			std::array<void*, 1> args = { object };
			invoke_result = static_cast<const nox::reflection::detail::FunctionInfoImpl<void>&>(function_info).InvokeImpl(args);
		}
		response->SetInvoked(invoke_result.has_value());
		return response;
	}

	NOX_ASSERT(false, u8"Inspector action function was not found: {0}", GetFunctionFullName());
	response->SetInvoked(false);
	return response;
}

nox::PlacementObject<nox::dev::editor_remote::Response> nox::dev::editor_remote::GetRuntimeDependencyGraphQuery::Execute(nox::World& world, std::span<nox::uint8> storage)const
{
	auto response = nox::PlacementObject<nox::dev::editor_remote::RuntimeDependencyGraphResponse>::Construct(storage);
#if !NOX_MASTER
	response->SetGraphText(world.BuildRuntimeDependencyGraphText());
#endif // !NOX_MASTER
	return response;
}

nox::PlacementObject<nox::dev::editor_remote::Response> nox::dev::editor_remote::GetRemoteInstanceSnapshotQuery::Execute(nox::World& world, std::span<nox::uint8> storage)const
{
	auto response = nox::PlacementObject<nox::dev::editor_remote::RemoteInstanceSnapshotResponse>::Construct(storage);
	nox::dev::editor_remote::EditorRemoteServer& server = world.GetSystem<nox::dev::editor_remote::EditorRemoteServerSystem>().GetServer();

	std::u8string text;
	text.reserve(3072);
	std::array<char, 512> line{};
	server.CollectRemoteInstances([&](nox::int64 instance_id, const nox::Object& object)
		{
			const nox::reflection::ClassInfo* const class_info = nox::reflection::FindClassInfo(object.GetType());
			const std::u8string_view full_name = class_info != nullptr ? class_info->GetFullName() : std::u8string_view(u8"Unknown");
			const std::u8string_view name = class_info != nullptr ? class_info->GetName() : std::u8string_view(u8"Unknown");
			const int length = std::snprintf(
				line.data(),
				line.size(),
				"INSTANCE|%lld|%.*s|%.*s|Active\n",
				static_cast<long long>(instance_id),
				static_cast<int>(full_name.size()),
				reinterpret_cast<const char*>(full_name.data()),
				static_cast<int>(name.size()),
				reinterpret_cast<const char*>(name.data()));
			if (text.size() + static_cast<std::size_t>(std::max(length, 0)) < 3000)
			{
				AppendAscii(text, line.data(), length);
			}
		});

	response->SetSnapshotText(text);
	return response;
}

nox::PlacementObject<nox::dev::editor_remote::Response> nox::dev::editor_remote::GetMemoryProfilerSnapshotQuery::Execute(nox::World&, std::span<nox::uint8> storage)const
{
	auto response = nox::PlacementObject<nox::dev::editor_remote::MemoryProfilerSnapshotResponse>::Construct(storage);

	std::u8string allocation_text;
	allocation_text.reserve(3072);
	std::array<char, 512> line{};
	nox::uint32 allocation_count = 0;
	nox::uint32 total_size = 0;
	nox::uint32 truncated_count = 0;
	const bool profile_enabled = nox::memory::profile::EnabledMemoryProfile();

	nox::memory::CollectHeapInfoList([&](const nox::memory::HeapInfo& heap_info)
		{
			++allocation_count;
			total_size += heap_info.size;

			std::size_t first_stack_address = 0;
			if (profile_enabled && heap_info.profile_handle != 0)
			{
				const nox::memory::profile::ProfileData& profile_data = nox::memory::profile::FindProfileData(heap_info.profile_handle);
				for (const std::size_t address : profile_data.call_stack_address_table)
				{
					if (address != 0)
					{
						first_stack_address = address;
						break;
					}
				}
			}

			const int length = std::snprintf(
				line.data(),
				line.size(),
				"ALLOC|%u|%u|%u|%s|%s|0x%zx\n",
				static_cast<unsigned>(heap_info.profile_handle),
				static_cast<unsigned>(heap_info.size),
				static_cast<unsigned>(heap_info.align_size),
				ToAscii(heap_info.instance_type),
				ToAscii(heap_info.segment_type),
				first_stack_address);
			if (allocation_text.size() + static_cast<std::size_t>(std::max(length, 0)) < 2800)
			{
				AppendAscii(allocation_text, line.data(), length);
			}
			else
			{
				++truncated_count;
			}
		});

	std::u8string text;
	text.reserve(3072);
	const int summary_length = std::snprintf(
		line.data(),
		line.size(),
		"SUMMARY|%u|%u|%u|%u\n",
		profile_enabled ? 1U : 0U,
		static_cast<unsigned>(allocation_count),
		static_cast<unsigned>(total_size),
		static_cast<unsigned>(truncated_count));
	AppendAscii(text, line.data(), summary_length);
	text += allocation_text;

	response->SetSnapshotText(text);
	return response;
}

nox::PlacementObject<nox::dev::editor_remote::Response>	nox::dev::editor_remote::EndSyncQuery::Execute(nox::World& world, std::span<nox::uint8> storage)const
{
	return nullptr;
}

#endif // NOX_DEVELOP
