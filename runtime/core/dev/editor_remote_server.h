//	Copyright (C) 2025 NOX ENGINE All rights reserved.

///	@file	remote_host.h
///	@brief	remote_host
#pragma once
#if NOX_DEVELOP
#include	"net/server.h"
#include	"net/client.h"
#include	"../engine_system.h"

#include	"socket_stream_writer.h"
#include	"socket_stream_reader.h"

namespace nox
{
	class Application;
	class Object;
	class ManagedObject;
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

	class EditorRemoteServer : public nox::dev::net::Server
	{
		NOX_DECLARE_OBJECT(nox::dev::editor_remote::EditorRemoteServer, nox::dev::net::Server);
		friend class EditorRemoteServerSystem;
	private:
		struct Impl;
	public:
		EditorRemoteServer();
		~EditorRemoteServer()override;

		void	SendQuery(nox::dev::editor_remote::Query& query, std::function<void(const nox::dev::editor_remote::Response&)> callback = nullptr);
		void	SendBuffer(std::span<const nox::uint8> buffer);
		
      void	RegisterRemoteInstance(nox::Object& object, nox::int64 instance_id = 0);
		void	RegisterEditorOwnedRemoteInstance(nox::ManagedObject& object, nox::int64 instance_id);
		bool	UnregisterRemoteInstance(nox::int64 instance_id);
		bool	NotifyRemoteInstanceDestroyed(nox::Object& object);

		nox::int64 FindRemoteInstanceId(const nox::Object& object)const noexcept;
		nox::Object* FindRemoteInstance(nox::int64 instance_id)const noexcept;
		void	CollectRemoteInstances(std::function<void(nox::int64, const nox::Object&)> evaluate)const;
	private:
		/// @brief main threadから呼び出される更新処理
		void	Start(nox::Application& application);
		void	Update(nox::Application& application);

		void	OnConnected(const nox::dev::net::ConnectionContext& context)override;
		void	OnDisconnected(const nox::dev::net::ConnectionContext& context)override;
		void UpdateReceive(nox::Application& application);
		void OnReceive(nox::Application& application);
	private:
		nox::uint32 query_id_counter_;
		nox::int64 instance_id_counter_;
		nox::UnorderedMap<nox::uint32, std::function<void(const nox::dev::editor_remote::Response&)>> response_dict_;
		nox::dev::editor_remote::SocketStreamWriter writer_;
		nox::dev::editor_remote::SocketStreamReader reader_;
		nox::dev::net::SocketScheduler* socket_scheduler_;

		/// @brief TODO:	現状は1つだけ対応
		nox::dev::net::ConnectionContext main_client_;

		nox::os::Mutex mutex_writer_;
		nox::os::Mutex mutex_reader_;

		/// @brief リモートインスタンス連想配列
		///	key:インスタンスID	正:エディタ側のインスタンス、負:Runtime側のインスタンス
		nox::UnorderedMap<nox::int64, std::reference_wrapper<nox::Object>> remote_instance_dict_;

		/// @brief リモートインスタンスIDを格納する辞書。Objectからint64へのマッピングを保持します。
		nox::UnorderedMap<const nox::Object*, nox::int64> remote_instance_id_dict_;

		NOX_ATTR(nox::reflection::attr::IgnoreReflection())
		nox::dev::editor_remote::EditorRemoteServer::Impl* impl_;
	};

	/// @brief 一時的なEditorRemoteServerのラッパー　EngineSystemとして登録するためのクラス
	///		@details 将来的にはEditorRemoteServer自体をEngineSystemとして実装する
	class EditorRemoteServerSystem : public nox::EngineSystem
	{
		NOX_DECLARE_OBJECT(nox::dev::editor_remote::EditorRemoteServerSystem, nox::EngineSystem);
	public:
		EditorRemoteServerSystem()noexcept :
			server_(new EditorRemoteServer()) {
		}
		
		~EditorRemoteServerSystem()override {
			
		}

		inline EditorRemoteServer& GetServer()const noexcept { return *this->server_; }

		std::span<const nox::EngineSystem::PhaseRegister> GetPhaseRegisterList()const noexcept override;
	private:
		inline void Initialize(nox::Application& application)
		{
			server_->Start(application);
		}

		inline void Update(nox::Application& application)
		{
			this->server_->Update(application);
		}

		inline void Terminate(nox::Application&)
		{
			delete server_;
			server_ = nullptr;
		}

	public:
		static constexpr SystemPhaseInit k_phase_init{
			&EditorRemoteServerSystem::Initialize,
			NOX_U8_NAMEOF_FUNCTION(&EditorRemoteServerSystem::Initialize)
		};

		static constexpr SystemPhaseUpdate k_phase_update{
			&EditorRemoteServerSystem::Update,
			NOX_U8_NAMEOF_FUNCTION(&EditorRemoteServerSystem::Update)
		};

		static constexpr SystemPhaseTerminate k_phase_terminate{
			&EditorRemoteServerSystem::Terminate,
			NOX_U8_NAMEOF_FUNCTION(&EditorRemoteServerSystem::Terminate)
		};

	private:
		EditorRemoteServer* server_;
		
	};
}
#endif // NOX_DEVELOP
