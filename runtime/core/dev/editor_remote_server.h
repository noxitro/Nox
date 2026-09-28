//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	remote_host.h
///	@brief	remote_host
#pragma once
#if NOX_DEVELOP
#include	"net/server.h"
#include	"net/client.h"
#include	"../service.h"
#include	"../service_attribute.h"

#include	"socket_stream_writer.h"
#include	"socket_stream_reader.h"

namespace nox
{
	class World;
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

	/// @brief		Editor との通信を受け持つService(開発ビルドのみ)。
	/// @details	OnInitialize で待ち受けの Server を起こして nox::dev::net::SocketScheduler の受信スレッドに登録し、
	///				OnShutdown で登録を外して閉じる。SocketScheduler は Depends に並べてあるので、
	///				初期化はこちらが後、終了はこちらが先になる(登録を外してから受信スレッドが止まる)。
	///
	///				受信スレッドは受け取ったデータを reader_ に積むだけで、World には触れない。
	///				積まれたクエリの実行は FrameIngress の UpdateReceive(nox::World& を取る排他ノード)が
	///				ゲームスレッド上で1フレームに1回行う。クエリは World を広く触る(SceneManager・entity など)ため、
	///				引数でアクセスを宣言する形では書けない。排他アクセスはこういう開発ツールの橋渡しのための逃げ道で、
	///				ゲームロジックでは使わない(nox::EntityParameterKind::World)。
	///
	///				他のノードから使うときは引数で受け取る。エンジン内部の Query の実装など、引数で受け取れない箇所は
	///				nox::World::TryGetService で引く。
	class EditorRemoteServer final:	public nox::Service
	{
		NOX_DECLARE_OBJECT(nox::dev::editor_remote::EditorRemoteServer, nox::Service);
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

			void OnServerReceive() override
			{
				owner_.OnServerReceive();
			}
		private:
			EditorRemoteServer& owner_;
		};

		struct Impl;
	public:
		/// @brief Server を受信スレッドに登録するので、SocketScheduler を先に初期化し、後に終了させる。
		using Depends = nox::TypeList<nox::dev::net::SocketScheduler>;

		EditorRemoteServer();
		~EditorRemoteServer()override;

		EditorRemoteServer(const EditorRemoteServer&) = delete;
		EditorRemoteServer& operator=(const EditorRemoteServer&) = delete;
		EditorRemoteServer(EditorRemoteServer&&) noexcept = delete;
		EditorRemoteServer& operator=(EditorRemoteServer&&) noexcept = delete;

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
		/// @brief 待ち受けを始め、SocketScheduler の受信スレッドに登録する。
		/// @details 待ち受けに失敗しても(ポートが使用中など)起動は止めない。Editor と繋がらないだけで、ゲームは動かせる。
		bool	OnInitialize(nox::ServiceContext& context)noexcept override;
		/// @brief 受信スレッドから登録を外し(外し終えるまで待つ)、待ち受けを閉じる。SocketScheduler はまだ生きている。
		void	OnShutdown()noexcept override;
		/// @brief 受信スレッドから登録を外し、待ち受けを閉じる。二重に呼んでも害はない(デストラクタからも呼ぶ)。
		/// @details 基底(nox::Service)の private な Shutdown と名前が重ならないよう別名にしてある。
		void	CloseServer();

		void	OnServerConnected(const nox::dev::net::ConnectionContext& context);
		void	OnServerDisconnected([[maybe_unused]] const nox::dev::net::ConnectionContext& context);
		/// @brief 受信スレッドから呼ばれる。受け取ったデータを reader_ に積むだけで、World には触れない。
		void	OnServerReceive();

		/// @brief 受信済みのクエリ / レスポンスを取り出して実行する。FrameIngress に1フレーム1回。
		/// @details nox::World& を取る排他ノード。同じフェーズの他のノードと並ばず単独で、
		///          フェーズを回しているスレッドで実行される。クエリは World を丸ごと触るため
		///          (Query::Execute が nox::World& を受け取る)。フェーズ実行中なので、クエリの中から
		///          即時系の構造変更(CreateEntity 等)は呼べない。
		NOX_ATTR(nox::attr::ServiceMethod(nox::SystemPhaseType::FrameIngress))
		void	UpdateReceive(nox::World& world);

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
}
#endif // NOX_DEVELOP
