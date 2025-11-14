//	Copyright (C) 2025 NOX ENGINE All rights reserved.

///	@file	remote_host.h
///	@brief	remote_host
#pragma once
#include	"net/server.h"
#include	"../object.h"

namespace nox::dev::editor_ipc
{
	class EditorIpcServer : public nox::dev::net::Server, public nox::ISingleton<EditorIpcServer>
	{
		NOX_DECLARE_OBJECT(nox::dev::editor_ipc::EditorIpcServer, nox::dev::net::Server);
	public:
		EditorIpcServer();
		~EditorIpcServer()override;

		void	SendQuery(class Query& query);

	private:
	};
}