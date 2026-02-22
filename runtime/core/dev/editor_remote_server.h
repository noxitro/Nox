//	Copyright (C) 2025 NOX ENGINE All rights reserved.

///	@file	remote_host.h
///	@brief	remote_host
#pragma once
#if NOX_DEVELOP
#include	"net/server.h"
#include	"../object.h"

#include	"socket_stream_writer.h"
#include	"socket_stream_reader.h"

namespace nox::dev::editor_remote
{
	class Query;
	class Response;

	/// @brief 同期モード
	enum class SyncMode : nox::uint8
	{
		/// @brief RemoteObjectをEditorObjectへ反映
		OneWay,
		/// @brief EditorObjectをRemoteObjectへ反映
		OneWaySource,
		/// @brief RemoteObjectとEditorObjectを双方向で反映
		TwoWay
	};

	class EditorRemoteServer : public nox::dev::net::Server, public nox::ISingleton<EditorRemoteServer>
	{
		NOX_DECLARE_OBJECT(nox::dev::editor_remote::EditorRemoteServer, nox::dev::net::Server);
	public:
		EditorRemoteServer();
		~EditorRemoteServer()override;

		void	SendQuery(nox::dev::editor_remote::Query& query, std::function<void(const nox::dev::editor_remote::Response&)> callback = nullptr);
		void	SendBuffer(std::span<const nox::uint8> buffer);

		/// @brief main threadから呼び出される更新処理
		void	Update();

		void	RegisterRemoteInstance(nox::Object& object, nox::int64 instance_id = 0);

		nox::int64 FindRemoteInstanceId(const nox::Object& object)const noexcept;
		nox::Object* FindRemoteInstance(nox::int64 instance_id)const noexcept;
	private:
		void	OnConnected(const nox::dev::net::ConnectionContext& context)override;
		void	OnDisconnected(const nox::dev::net::ConnectionContext& context)override;
		void UpdateReceive();
		void OnReceive();
	private:
		nox::uint32 query_id_counter_;
		nox::int64 instance_id_counter_;
		nox::UnorderedMap<nox::uint32, std::function<void(const nox::dev::editor_remote::Response&)>> response_dict_;
		nox::dev::editor_remote::SocketStreamWriter writer_;
		nox::dev::editor_remote::SocketStreamReader reader_;

		/// @brief TODO:	現状は1つだけ対応
		nox::dev::net::ConnectionContext main_client_;

		nox::os::Mutex mutex_writer_;

		/// @brief リモートインスタンス連想配列
		///	key:インスタンスID	正:エディタ側のインスタンス、負:Runtime側のインスタンス
		nox::UnorderedMap<nox::int64, std::reference_wrapper<nox::Object>> remote_instance_dict_;

		/// @brief リモートインスタンスIDを格納する辞書。Objectからint64へのマッピングを保持します。
		nox::UnorderedMap<const nox::Object*, nox::int64> remote_instance_id_dict_;
	};
}
#endif // NOX_DEVELOP