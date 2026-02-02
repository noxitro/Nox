//	Copyright (c) 2025 NOX ENGINE All rights reserved.
//	do not edit
//	written from RuntimeRemoteCodeGenerator

#include	"stdafx.h"
#include	"system.g.h"
#include	"../socket_stream_writer.h"
#include	"../socket_stream_reader.h"

void nox::dev::editor_ipc::ResourceConvertQuery::OnSerialize(nox::dev::editor_ipc::SocketStreamWriter& writer)
{
	writer.Write(nativePath_);
}

void nox::dev::editor_ipc::ResourceConvertQuery::OnDeserialize(nox::dev::editor_ipc::SocketStreamReader& reader)
{
	reader.Read(nativePath_);
}
