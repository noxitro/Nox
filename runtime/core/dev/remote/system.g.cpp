//	Copyright (c) 2026 NOX ENGINE All rights reserved.
//	do not edit
//	written from RuntimeRemoteCodeGenerator

#include	"stdafx.h"
#include	"system.g.h"
#include	"../socket_stream_writer.h"
#include	"../socket_stream_reader.h"
#include	"../../scene_view.h"

void nox::dev::editor_ipc::ResourceConvertQuery::OnSerialize(nox::dev::editor_ipc::SocketStreamWriter& writer)
{
	writer.Write(native_path_);
}

void nox::dev::editor_ipc::ResourceConvertQuery::OnDeserialize(nox::dev::editor_ipc::SocketStreamReader& reader)
{
//	reader.Read(native_path_);
}
void nox::dev::editor_ipc::SceneViewInfo::OnSerialize(nox::dev::editor_ipc::SocketStreamWriter& writer)
{
	writer.Write(main_window_handle_);
//	writer.Write(scene_view_);
}

void nox::dev::editor_ipc::SceneViewInfo::OnDeserialize(nox::dev::editor_ipc::SocketStreamReader& reader)
{
	const nox::IntrusivePtr<nox::reflection::ReflectionObject>& temp_scene_view = scene_view_;

//	main_window_handle_ = reader.Read(main_window_handle_);


	const nox::reflection::ReflectionObject* pp = scene_view_.Get();
	reader.Read(scene_view_);
}
