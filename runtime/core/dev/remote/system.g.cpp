//	Copyright (c) 2026 NOX ENGINE All rights reserved.
//	do not edit
//	written from RuntimeRemoteCodeGenerator

#include	"pch.h"
#include	"system.g.h"
#include	"codegen_preamble.h"


void nox::dev::editor_remote::ResourceConvertQuery::OnSerialize(nox::dev::editor_remote::SocketStreamWriter& writer)
{
	writer.Write(native_path_);
}

void nox::dev::editor_remote::ResourceConvertQuery::OnDeserialize(nox::dev::editor_remote::SocketStreamReader& reader)
{
	reader.Read(native_path_);
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
