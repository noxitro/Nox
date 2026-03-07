//	Copyright (c) 2026 NOX ENGINE All rights reserved.
//	do not edit
//	written from RuntimeRemoteCodeGenerator

#include	"stdafx.h"
#include	"log.g.h"
#include	"codegen_preamble.h"


void nox::dev::editor_remote::SendLog::OnSerialize(nox::dev::editor_remote::SocketStreamWriter& writer)
{
	writer.Write(level_);
}

void nox::dev::editor_remote::SendLog::OnDeserialize(nox::dev::editor_remote::SocketStreamReader& reader)
{
	reader.Read(level_);
}
