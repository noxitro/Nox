//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	server.h
///	@brief	server
#pragma once
#include	"net_entity.h"

namespace nox::dev::net
{
	/// @brief Server のイベントの通知先。
	/// @details いずれも nox::dev::net::SocketScheduler の受信スレッドから呼ばれる。
	///          受信スレッドは World に触れないので、受け取ったデータをフレームへ取り込むのは
	///          通知先の持ち主(EditorRemoteServer の FrameIngress のメソッドなど)の役目。
	class IServerEventHandler
	{
	public:
		virtual void OnServerConnected(const nox::dev::net::ConnectionContext& context) = 0;
		virtual void OnServerDisconnected(const nox::dev::net::ConnectionContext& context) = 0;
		/// @brief 接続中のクライアントのソケットに読めるデータがある。
		virtual void OnServerReceive() = 0;
	protected:
		virtual ~IServerEventHandler() = default;
	};

	class Server
	{
	public:
		struct InitializeContext
		{
			nox::uint16 max_connection;
			nox::dev::net::port_t port;
			bool resume_port = false;
		};

	private:

		enum class Flag : nox::uint8
		{
			None = 0x00,
			Startup = 0x01,
			Shutdown = 0x02,
			Error = 0x04,
		};

		struct PendingPeerContext
		{
			nox::dev::net::raw_socket_t  socket;
			ConnectionContext connection;
			ConnectionState connection_state = ConnectionState::Invalid;
		};
	public:
		explicit Server(nox::dev::net::IServerEventHandler& event_handler)noexcept;
		~Server();

		Server(const Server&) = delete;
		Server& operator=(const Server&) = delete;
		Server(Server&&) noexcept = delete;
		Server& operator=(Server&&) noexcept = delete;

		bool Startup(const Server::InitializeContext& context);
		void Shutdown();

		inline bool IsStartup()const noexcept { return is_startup_; }
		//void PollAccept();

		/// @brief 接続処理
		/// @param fd 
		void	Connection(::fd_set& fds);

		/// @brief SocketSchedulerの受信スレッドから呼び出される更新処理
		void Update();

		/// @brief clientを切断
		/// @param socket 切断するclientのソケット
		void Disconnect(const nox::dev::net::raw_socket_t socket);

		inline constexpr nox::dev::net::raw_socket_t GetSocket()const noexcept { return socket_; }

		std::expected<void, nox::dev::net::SocketIoError> Send(
			nox::dev::net::raw_socket_t socket,
			nox::not_null<const void*> buffer,
			nox::int32 size_to_send,
			nox::dev::net::SendFlag flag = nox::dev::net::SendFlag::None);

		template<nox::concepts::TriviallyCopyable T>
		inline std::expected<void, nox::dev::net::SocketIoError> Send(
			nox::dev::net::raw_socket_t socket,
			const T& buffer,
			nox::dev::net::SendFlag flag = nox::dev::net::SendFlag::None)
		{
			return this->Send(socket, static_cast<const void*>(&buffer), static_cast<nox::int32>(sizeof(T)), flag);
		}
	private:
		nox::dev::net::IServerEventHandler& event_handler_;
		Server::InitializeContext initialize_context_;
		nox::Vector<PeerContext> client_list_;

		/// @brief 
		nox::Vector<nox::dev::net::raw_socket_t> detached_client_list_;
		nox::dev::net::raw_socket_t socket_;

		bool is_startup_:1;

#if !NOX_MASTER
		nox::util::ParallelExecuteChecker pe_checker_clients_;
#endif // !NOX_MASTER
	};
}