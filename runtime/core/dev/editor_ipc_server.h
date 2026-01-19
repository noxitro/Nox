//	Copyright (C) 2025 NOX ENGINE All rights reserved.

///	@file	remote_host.h
///	@brief	remote_host
#pragma once
#if NOX_DEVELOP
#include	"net/server.h"
#include	"../object.h"

#include	"socket_stream_writer.h"
#include	"socket_stream_reader.h"

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

		void	SendQuery(nox::dev::editor_ipc::Query& query, std::function<void(const nox::dev::editor_ipc::Response&)> callback = nullptr);
		void	SendBuffer(std::span<const nox::uint8> buffer);

		/// @brief main threadから呼び出される更新処理
		void	Update();
	private:
		void	OnConnected(const nox::dev::net::ConnectionContext& context)override;
		void	OnDisconnected(const nox::dev::net::ConnectionContext& context)override;
		void UpdateReceive();
		void OnReceive();
	private:
		nox::uint32 query_id_counter_;
		nox::UnorderedMap<nox::uint32, std::function<void(const nox::dev::editor_ipc::Response&)>> response_dict_;
		nox::dev::editor_ipc::SocketStreamWriter writer_;
		nox::dev::editor_ipc::SocketStreamReader reader_;

		/// @brief TODO:	現状は1つだけ対応
		nox::dev::net::ConnectionContext main_client_;

		nox::os::Mutex mutex_writer_;
	};
}
#endif // NOX_DEVELOP