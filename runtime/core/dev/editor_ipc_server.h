//	Copyright (C) 2025 NOX ENGINE All rights reserved.

///	@file	remote_host.h
///	@brief	remote_host
#pragma once
#if NOX_DEVELOP
#include	"net/server.h"
#include	"../object.h"

namespace nox::dev::editor_ipc
{
	class Query;
	class Response;

	class EditorIpcServer : public nox::dev::net::Server, public nox::ISingleton<EditorIpcServer>
	{
		NOX_DECLARE_OBJECT(nox::dev::editor_ipc::EditorIpcServer, nox::dev::net::Server);
	public:
		EditorIpcServer();
		~EditorIpcServer()override;

		void	SendQuery(nox::dev::editor_ipc::Query& query, std::function<nox::dev::editor_ipc::Response&()> callback = nullptr);

	private:
	};
}
#endif // NOX_DEVELOP