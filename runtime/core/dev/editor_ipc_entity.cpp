//	Copyright (C) 2024 NOX ENGINE All Rights Rserved.

///	@file	editor_ipc_entity.cpp
///	@brief	editor_ipc_entity
#include	"stdafx.h"
#include	"editor_ipc_entity.h"

#include	"editor_ipc_client.h"
#include	"socket_stream_writer.h"
#include	"socket_stream_reader.h"

void nox::dev::editor_ipc::EditorIpcEntity::Serialize(const nox::uint32 id, SocketStreamWriter& writer)
{
	const nox::reflection::Type& type = GetType();
	const nox::reflection::ClassInfo* const class_info = nox::reflection::FindClassInfo(type);
	NOX_ASSERT(class_info != nullptr, u"不明なEditorIpcEntity:{0}", type.GetTypeName());

	writer.Write(class_info->GetFullName());

	writer.Write(id);
	this->OnSerialize(writer);
}

void nox::dev::editor_ipc::EditorIpcEntity::Deserialize(SocketStreamReader& reader)
{
	id_ = reader.Read<decltype(id_)>();
	this->OnDeserialize(reader);
}