//	Copyright (c) 2025 NOX ENGINE All rights reserved.

///	@file	remote_host.cpp
///	@brief	remote_host
#include	"stdafx.h"
#include	"editor_ipc_server.h"

#include	"editor_ipc_query.h"

namespace nox::dev::editor_ipc
{
	nox::uint32 query_handle_counter_ = 0;

	inline nox::uint32 IssueQueryHandle() noexcept
	{
		return nox::os::atomic::Increment(query_handle_counter_);
	}
}

nox::dev::editor_ipc::EditorIpcServer::EditorIpcServer()
{
	this->Startup(InitializeContext{
		.max_connection = 1,
		.port = 0,
		});
}

nox::dev::editor_ipc::EditorIpcServer::~EditorIpcServer()
{

}

void	nox::dev::editor_ipc::EditorIpcServer::SendQuery(nox::dev::editor_ipc::Query& query)
{

}