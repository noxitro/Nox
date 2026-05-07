//	Copyright (c) 2026 NOX ENGINE All rights reserved.
//	do not edit
//	written from RuntimeRemoteCodeGenerator

#include	"pch.h"
#include	"log.g.h"
#include	"codegen_preamble.h"


void nox::dev::editor_remote::SendLog::OnSerialize(nox::dev::editor_remote::SocketStreamWriter& writer)
{
	writer.Write(level_);
	writer.Write(msg_);
	writer.Write(call_stack_);
	writer.Write(channel_);
}

void nox::dev::editor_remote::SendLog::OnDeserialize(nox::dev::editor_remote::SocketStreamReader& reader)
{
	NOX_ASSERT(false, u8"送信専用Queryです");
}
