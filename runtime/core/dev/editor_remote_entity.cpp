//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	editor_remote_entity.cpp
///	@brief	editor_remote_entity
#include	"pch.h"
#if NOX_DEVELOP
#include	"editor_remote_entity.h"

#include	"editor_remote_client.h"
#include	"socket_stream_writer.h"
#include	"socket_stream_reader.h"

void nox::dev::editor_remote::EditorRemoteEntity::Serialize(const nox::uint32 id, SocketStreamWriter& writer)
{
	const nox::reflection::Type& type = GetType();
	const nox::reflection::ClassInfo* const class_info = nox::reflection::FindClassInfo(type);
	NOX_ASSERT(class_info != nullptr, u"不明なEditorRemoteEntity:{0}", type.GetTypeName());

	writer.Write(class_info->GetFullName());

	writer.Write(id);
	this->OnSerialize(writer);
}

void nox::dev::editor_remote::EditorRemoteEntity::Deserialize(SocketStreamReader& reader)
{
	reader.Read(id_);
	this->OnDeserialize(reader);
}
#endif // NOX_DEVELOP
