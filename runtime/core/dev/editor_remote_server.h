//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	remote_host.h
///	@brief	remote_host
#pragma once
#if NOX_DEVELOP
#include	"net/server.h"
#include	"net/client.h"
#include	"../service.h"
#include	"../ecs_definitions.h"

#include	"socket_stream_writer.h"
#include	"socket_stream_reader.h"

namespace nox
{
	class Object;
}

namespace nox::dev::net
{
	class SocketScheduler;
}

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

	class EditorRemoteServer final : public nox::Service<EditorRemoteServer>
	{
		NOX_ECS_DECLARE_VERIFY(EditorRemoteServer);
	private:
		class ServerEventHandler final : public nox::dev::net::IServerEventHandler
		{
		public:
			explicit ServerEventHandler(EditorRemoteServer& owner)noexcept :
				owner_(owner) {
			}

			void OnServerConnected(const nox::dev::net::ConnectionContext& context) override
			{
				owner_.OnServerConnected(context);
			}

			void OnServerDisconnected(const nox::dev::net::ConnectionContext& context) override
			{
				owner_.OnServerDisconnected(context);
			}

			void OnServerReceive(nox::legacy::World& world) override
			{
				owner_.OnServerReceive(world);
			}
		private:
			EditorRemoteServer& owner_;
		};

		struct Impl;
	public:
		EditorRemoteServer();
		~EditorRemoteServer();

		void	SendQuery(nox::dev::editor_remote::Query& query, std::function<void(const nox::dev::editor_remote::Response&)> callback = nullptr);
		void	SendBuffer(std::span<const nox::uint8> buffer);
		
      void	RegisterRemoteInstance(nox::Object& object, nox::int64 instance_id = 0);
		void	RegisterEditorOwnedRemoteInstance(nox::Object& object, nox::int64 instance_id);
		bool	UnregisterRemoteInstance(nox::int64 instance_id);
		bool	NotifyRemoteInstanceDestroyed(nox::Object& object);

		nox::int64 FindRemoteInstanceId(const nox::Object& object)const noexcept;
		nox::Object* FindRemoteInstance(nox::int64 instance_id)const noexcept;
		void	CollectRemoteInstances(std::function<void(nox::int64, const nox::Object&)> evaluate)const;
	private:
		/// @brief main threadから呼び出される更新処理
		void	Start(nox::legacy::World& world);
		void	Shutdown();
		void	Update(nox::legacy::World& world);
		void	Terminate(nox::legacy::World& world);

		void	OnServerConnected(const nox::dev::net::ConnectionContext& context);
		void	OnServerDisconnected([[maybe_unused]] const nox::dev::net::ConnectionContext& context);
		void	OnServerReceive(nox::legacy::World& world);
		void UpdateReceive(nox::legacy::World& world);
	public:
		static constexpr PhaseInit kPhaseInit{ &EditorRemoteServer::Start };
		static constexpr PhaseUpdate kPhaseUpdate{ &EditorRemoteServer::Update };
		static constexpr PhaseTerminate kPhaseTerminate{ &EditorRemoteServer::Terminate };

		static const NodeDescriptor& GetNodeDescriptor()noexcept;
	private:
		ServerEventHandler event_handler_;
		nox::dev::net::Server server_;
		nox::uint32 query_id_counter_;
		nox::int64 instance_id_counter_;
		nox::UnorderedMap<nox::uint32, std::function<void(const nox::dev::editor_remote::Response&)>> response_dict_;
		nox::dev::editor_remote::SocketStreamWriter writer_;
		nox::dev::editor_remote::SocketStreamReader reader_;
		nox::dev::net::SocketScheduler* socket_scheduler_;

		/// @brief TODO:	現状は1つだけ対応
		nox::dev::net::ConnectionContext main_client_;

		nox::Mutex mutex_writer_;
		nox::Mutex mutex_reader_;

		/// @brief リモートインスタンス連想配列
		///	key:インスタンスID	正:エディタ側のインスタンス、負:Runtime側のインスタンス
		nox::UnorderedMap<nox::int64, std::reference_wrapper<nox::Object>> remote_instance_dict_;

		/// @brief リモートインスタンスIDを格納する辞書。Objectからint64へのマッピングを保持します。
		nox::UnorderedMap<const nox::Object*, nox::int64> remote_instance_id_dict_;

		NOX_ATTR(nox::reflection::attr::IgnoreReflection())
		nox::dev::editor_remote::EditorRemoteServer::Impl* impl_;
	};
}
#endif // NOX_DEVELOP
