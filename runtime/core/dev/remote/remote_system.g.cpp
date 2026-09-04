//	Copyright (c) 2026 NOX ENGINE All rights reserved.
//	do not edit
//	written from RuntimeRemoteCodeGenerator

#include	"pch.h"
#if	NOX_DEVELOP
#include	"remote_system.g.h"
#include	"codegen_preamble.h"


void nox::dev::editor_remote::AssetConvertQuery::OnSerialize(nox::dev::editor_remote::SocketStreamWriter& writer)
{
	writer.Write(uri_);
}

void nox::dev::editor_remote::AssetConvertQuery::OnDeserialize(nox::dev::editor_remote::SocketStreamReader& reader)
{
	reader.Read(uri_);
}
void nox::dev::editor_remote::SceneViewInfo::OnSerialize(nox::dev::editor_remote::SocketStreamWriter& writer)
{
	writer.Write(main_window_handle_);
}

void nox::dev::editor_remote::SceneViewInfo::OnDeserialize(nox::dev::editor_remote::SocketStreamReader& reader)
{
	reader.Read(main_window_handle_);
}
void nox::dev::editor_remote::SyncQuery::OnSerialize(nox::dev::editor_remote::SocketStreamWriter& writer)
{
	writer.Write(remote_instance_id_);
	writer.Write(fqn_);
	writer.Write(property_byte_buffer_);
}

void nox::dev::editor_remote::SyncQuery::OnDeserialize(nox::dev::editor_remote::SocketStreamReader& reader)
{
	reader.Read(remote_instance_id_);
	reader.Read(fqn_);
	reader.Read(property_byte_buffer_);
}
void nox::dev::editor_remote::SyncResponse::OnSerialize(nox::dev::editor_remote::SocketStreamWriter& writer)
{
	writer.Write(remote_instance_id_);
	writer.Write(applied_);
}

void nox::dev::editor_remote::SyncResponse::OnDeserialize(nox::dev::editor_remote::SocketStreamReader& reader)
{
	reader.Read(remote_instance_id_);
	reader.Read(applied_);
}
void nox::dev::editor_remote::AddEntityNodeQuery::OnSerialize(nox::dev::editor_remote::SocketStreamWriter& writer)
{
	writer.Write(remote_instance_id_);
	writer.Write(parent_remote_instance_id_);
	writer.Write(name_);
}

void nox::dev::editor_remote::AddEntityNodeQuery::OnDeserialize(nox::dev::editor_remote::SocketStreamReader& reader)
{
	reader.Read(remote_instance_id_);
	reader.Read(parent_remote_instance_id_);
	reader.Read(name_);
}
void nox::dev::editor_remote::AddEntityNodeResponse::OnSerialize(nox::dev::editor_remote::SocketStreamWriter& writer)
{
	writer.Write(remote_instance_id_);
	writer.Write(created_);
	writer.Write(attached_);
}

void nox::dev::editor_remote::AddEntityNodeResponse::OnDeserialize(nox::dev::editor_remote::SocketStreamReader& reader)
{
	reader.Read(remote_instance_id_);
	reader.Read(created_);
	reader.Read(attached_);
}
void nox::dev::editor_remote::AddComponentQuery::OnSerialize(nox::dev::editor_remote::SocketStreamWriter& writer)
{
	writer.Write(remote_instance_id_);
	writer.Write(entity_node_remote_instance_id_);
	writer.Write(component_type_fqn_);
	writer.Write(property_byte_buffer_);
}

void nox::dev::editor_remote::AddComponentQuery::OnDeserialize(nox::dev::editor_remote::SocketStreamReader& reader)
{
	reader.Read(remote_instance_id_);
	reader.Read(entity_node_remote_instance_id_);
	reader.Read(component_type_fqn_);
	reader.Read(property_byte_buffer_);
}
void nox::dev::editor_remote::AddComponentResponse::OnSerialize(nox::dev::editor_remote::SocketStreamWriter& writer)
{
	writer.Write(remote_instance_id_);
	writer.Write(created_);
	writer.Write(added_);
}

void nox::dev::editor_remote::AddComponentResponse::OnDeserialize(nox::dev::editor_remote::SocketStreamReader& reader)
{
	reader.Read(remote_instance_id_);
	reader.Read(created_);
	reader.Read(added_);
}
void nox::dev::editor_remote::DestroyEntityNodeQuery::OnSerialize(nox::dev::editor_remote::SocketStreamWriter& writer)
{
	writer.Write(remote_instance_id_);
}

void nox::dev::editor_remote::DestroyEntityNodeQuery::OnDeserialize(nox::dev::editor_remote::SocketStreamReader& reader)
{
	reader.Read(remote_instance_id_);
}
void nox::dev::editor_remote::AutoSyncQuery::OnSerialize(nox::dev::editor_remote::SocketStreamWriter& writer)
{
	writer.Write(remote_instance_id_);
}

void nox::dev::editor_remote::AutoSyncQuery::OnDeserialize(nox::dev::editor_remote::SocketStreamReader& reader)
{
	reader.Read(remote_instance_id_);
}
void nox::dev::editor_remote::AutoSyncResponse::OnSerialize(nox::dev::editor_remote::SocketStreamWriter& writer)
{
	writer.Write(remote_instance_id_);
	writer.Write(exists_);
	writer.Write(property_byte_buffer_);
}

void nox::dev::editor_remote::AutoSyncResponse::OnDeserialize(nox::dev::editor_remote::SocketStreamReader& reader)
{
	reader.Read(remote_instance_id_);
	reader.Read(exists_);
	reader.Read(property_byte_buffer_);
}
void nox::dev::editor_remote::InvokeRuntimeActionQuery::OnSerialize(nox::dev::editor_remote::SocketStreamWriter& writer)
{
	writer.Write(remote_instance_id_);
	writer.Write(function_full_name_);
}

void nox::dev::editor_remote::InvokeRuntimeActionQuery::OnDeserialize(nox::dev::editor_remote::SocketStreamReader& reader)
{
	reader.Read(remote_instance_id_);
	reader.Read(function_full_name_);
}
void nox::dev::editor_remote::InvokeRuntimeActionResponse::OnSerialize(nox::dev::editor_remote::SocketStreamWriter& writer)
{
	writer.Write(remote_instance_id_);
	writer.Write(invoked_);
}

void nox::dev::editor_remote::InvokeRuntimeActionResponse::OnDeserialize(nox::dev::editor_remote::SocketStreamReader& reader)
{
	reader.Read(remote_instance_id_);
	reader.Read(invoked_);
}
void nox::dev::editor_remote::RuntimeDependencyGraphResponse::OnSerialize(nox::dev::editor_remote::SocketStreamWriter& writer)
{
	writer.Write(graph_text_);
}

void nox::dev::editor_remote::RuntimeDependencyGraphResponse::OnDeserialize(nox::dev::editor_remote::SocketStreamReader& reader)
{
	reader.Read(graph_text_);
}
void nox::dev::editor_remote::RemoteInstanceSnapshotResponse::OnSerialize(nox::dev::editor_remote::SocketStreamWriter& writer)
{
	writer.Write(snapshot_text_);
}

void nox::dev::editor_remote::RemoteInstanceSnapshotResponse::OnDeserialize(nox::dev::editor_remote::SocketStreamReader& reader)
{
	reader.Read(snapshot_text_);
}
void nox::dev::editor_remote::MemoryProfilerSnapshotResponse::OnSerialize(nox::dev::editor_remote::SocketStreamWriter& writer)
{
	writer.Write(snapshot_text_);
}

void nox::dev::editor_remote::MemoryProfilerSnapshotResponse::OnDeserialize(nox::dev::editor_remote::SocketStreamReader& reader)
{
	reader.Read(snapshot_text_);
}
void nox::dev::editor_remote::RuntimeObjectDestroyedQuery::OnSerialize(nox::dev::editor_remote::SocketStreamWriter& writer)
{
	writer.Write(remote_instance_id_);
}

void nox::dev::editor_remote::RuntimeObjectDestroyedQuery::OnDeserialize(nox::dev::editor_remote::SocketStreamReader& reader)
{
	NOX_ASSERT(false, u8"送信専用Queryです");
}
void nox::dev::editor_remote::EndSyncQuery::OnSerialize(nox::dev::editor_remote::SocketStreamWriter& writer)
{
	writer.Write(remote_instance_id_);
}

void nox::dev::editor_remote::EndSyncQuery::OnDeserialize(nox::dev::editor_remote::SocketStreamReader& reader)
{
	reader.Read(remote_instance_id_);
}
#endif	//	NOX_DEVELOP
