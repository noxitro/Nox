//	Copyright (C) 2025 NOX ENGINE All rights reserved.

///	@file	server.h
///	@brief	server
#pragma once
#include	"entity.h"

namespace nox::dev::net
{
	class Server : public nox::dev::net::Entity
	{
		NOX_DECLARE_OBJECT(nox::dev::net::Server, nox::dev::net::Entity);
	public:
		struct InitializeContext
		{
			nox::uint16 max_connection;
			nox::dev::net::port_t port;
			bool resume_port;
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
		Server();
		~Server()override;
		bool Startup(const Server::InitializeContext& context);
		void Shutdown();

		inline bool IsStartup()const noexcept { return is_startup_; }
		//void PollAccept();

		/// @brief 接続処理
		/// @param fd 
		void	Connection(::fd_set& fds);

		/// @brief SocketSchedulerから呼び出される更新処理
		void Update();

		/// @brief clientを切断
		/// @param socket 切断するclientのソケット
		void Disconnect(const nox::dev::net::raw_socket_t socket);

		inline constexpr nox::dev::net::raw_socket_t GetSocket()const noexcept { return socket_; }
	protected:
		void Connected([[maybe_unused]] const nox::dev::net::ConnectionContext& context);
		virtual void OnConnected([[maybe_unused]] const nox::dev::net::ConnectionContext& context) {}

		void Disconnected([[maybe_unused]] const nox::dev::net::ConnectionContext& context);
		virtual void OnDisconnected([[maybe_unused]] const nox::dev::net::ConnectionContext& context) {}

		inline const nox::Vector<PeerContext>& GetClientList()const noexcept { return client_list_; }
	private:
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