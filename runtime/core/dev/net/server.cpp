//	Copyright (c) 2025 NOX ENGINE All rights reserved.

///	@file	server.cpp
///	@brief	server
#include	"pch.h"
#include	"server.h"

#include	"socket_scheduler.h"
#include	"dev_net_api.h"
#include	"dev_net_log_id.h"

namespace nox::dev::net
{
	inline constexpr std::string_view k_hand_shake_str1 = "HandShake1";
	inline constexpr std::string_view k_hand_shake_str2 = "HandShake2";
	inline constexpr std::string_view k_hand_shake_str3 = "HandShake3";
}

nox::dev::net::Server::Server() :
	is_startup_(false),
	socket_(nox::dev::net::k_raw_invalid_socket),
	initialize_context_{}
{

}

nox::dev::net::Server::~Server()
{
	this->Shutdown();
}

bool nox::dev::net::Server::Startup(const InitializeContext& context)
{
	if (IsStartup()==true)
	{
		this->Shutdown();
	}
	const nox::int32 err = ::WSAGetLastError();
	initialize_context_ = context;

	//PollAccept();

#if NOX_WINDOWS
	raw_sockaddr_in ip_address;
	ip_address.sin_family = AF_INET;
	ip_address.sin_addr.S_un.S_addr = INADDR_ANY;	//	全てのマシンを受け付け
	ip_address.sin_port = ::htons(static_cast<nox::uint16>(this->initialize_context_.port));
	this->socket_ = ::socket(ip_address.sin_family, SOCK_STREAM, 0);

	NOX_ASSERT(socket_ != INVALID_SOCKET, nox::util::Format(u"socket() failed. error_code={0}", ::WSAGetLastError()));

	//	非ブロッキング
	::u_long nb = 1;
	if (::ioctlsocket(this->socket_, FIONBIO, &nb) == k_raw_error_socket)
	{
		NOX_ASSERT(false, nox::util::Format(u"ioctlsocket() failed. error_code={0}", ::WSAGetLastError()));
	}

	//	遅延を減らす
	nox::dev::net::SetNoDelay(this->socket_, true);

	//TODO:	ネットワークレイテンシの向上
#endif // NOX_WINDOWS

	//	socketにアドレスを割り当て
	nox::dev::net::Bind(this->socket_, *reinterpret_cast<const nox::dev::net::raw_sockaddr*>(&ip_address), sizeof(ip_address));

	//	接続待ち状態にする
	if (nox::dev::net::Listen(this->socket_, this->initialize_context_.max_connection) == k_raw_error_socket)
	{
		NOX_ERROR_LINE(nox::dev::net::log_id::DevNet, u"Listenに失敗しました");
		return false;
	}

	is_startup_ = true;
	nox::dev::net::SocketScheduler::Instance().RegisterEntity(*this);
	return true;
}

void	nox::dev::net::Server::Connection(::fd_set& fds)
{
	if (IsStartup() == false)
	{
		return;
	}

	if (!nox::os::file_descriptor::IsSet(this->socket_, fds))
	{
		return;
	}

	constexpr ::timeval timeout
	{
		.tv_sec = 0,
		.tv_usec = 10000
	};

	while (true)
	{
		//	受け付けられる接続が無くなるまでacceptを繰り返す
		nox::dev::net::raw_sockaddr_in addr{};
		int client_len = sizeof(addr);
		const nox::dev::net::raw_socket_t client_socket = nox::dev::net::Accept(this->socket_, reinterpret_cast<nox::dev::net::raw_sockaddr&>(addr), client_len);
		if (client_socket == nox::dev::net::k_raw_invalid_socket)
		{
			const int error_code = ::WSAGetLastError();
			if (error_code == WSAEWOULDBLOCK || error_code == WSAEINTR)
			{
				//	接続待ち無し
				break;
			}
			NOX_ERROR_LINE(nox::dev::net::log_id::DevNet, nox::util::Format(u"Acceptに失敗しました。error_code={0}", error_code));
			break;
		}

		// 接続ソケットに低遅延/KeepAlive等の推奨オプションを適用（任意）
		{
			::u_long nb = 1;
			::ioctlsocket(client_socket, FIONBIO, &nb);
			int on = 1;

			nox::dev::net::SetNoDelay(client_socket, true);

			::setsockopt(client_socket, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char*>(&on), sizeof(on));
			::setsockopt(client_socket, SOL_SOCKET, SO_KEEPALIVE, reinterpret_cast<const char*>(&on), sizeof(on));
		}

		bool success = false;
		NOX_LOCAL_SCOPE(nox::util::ScopeExit([&success, &client_socket]() {
			if (!success)
			{
				::closesocket(client_socket);
			}
			}));

		//	handshake1の受信
		{
			::fd_set read_fds{};
			::fd_set except_fds{};

			nox::os::file_descriptor::Zero(read_fds);
			nox::os::file_descriptor::Zero(except_fds);

			nox::os::file_descriptor::Set(client_socket, read_fds);
			nox::os::file_descriptor::Set(client_socket, except_fds);

			const auto select_result = ::select(0, &read_fds, nullptr, &except_fds, &timeout);

			if (select_result == nox::dev::net::k_raw_error_socket)
			{
				const int err = ::WSAGetLastError();
				NOX_ERROR_LINE(nox::dev::net::log_id::DevNet, nox::util::Format(u"select(handshake1) failed. error_code={0}", err));
				break;
			}

			if (select_result == 0)
			{
				NOX_ERROR_LINE(nox::dev::net::log_id::DevNet, u"timeout handshake1");
				break;
			}

			if (nox::os::file_descriptor::IsSet(client_socket, except_fds) == true)
			{
				NOX_ERROR_LINE(nox::dev::net::log_id::DevNet, u"exception handshake1");
				break;
			}

			if (nox::os::file_descriptor::IsSet(client_socket, read_fds) == false)
			{
				NOX_ERROR_LINE(nox::dev::net::log_id::DevNet, u"invalid socket handshake1");
				break;
			}

			std::array<char, k_hand_shake_str1.length()> dest_buffer{0};
			auto r = this->Receive(client_socket, dest_buffer.data(), static_cast<nox::int32>(k_hand_shake_str1.length()));
			if (!r)
			{
				NOX_ERROR_LINE(nox::dev::net::log_id::DevNet, u"ハンドシェイク1の受信に失敗しました");
				break;
			}

			if (std::string_view(dest_buffer.data(), dest_buffer.size()) != k_hand_shake_str1)
			{
				NOX_ERROR_LINE(nox::dev::net::log_id::DevNet, u"ハンドシェイク1の内容が不正です");
				break;
			}
		}
		
		//	handshake2の送信
		{
			const auto r = this->Send(client_socket, k_hand_shake_str2.data(), static_cast<nox::int32>(k_hand_shake_str2.length()));
			if (!r)
			{
				NOX_ERROR_LINE(nox::dev::net::log_id::DevNet, u"ハンドシェイク2の送信に失敗しました");
				break;
			}
		}

		//	handshake3の受信
		{
			::fd_set read_fds{};
			::fd_set except_fds{};

			nox::os::file_descriptor::Zero(read_fds);
			nox::os::file_descriptor::Zero(except_fds);

			nox::os::file_descriptor::Set(client_socket, read_fds);
			nox::os::file_descriptor::Set(client_socket, except_fds);

			const auto select_result = ::select(0, &read_fds, nullptr, &except_fds, &timeout);

			if (select_result == nox::dev::net::k_raw_error_socket)
			{
				const int err = ::WSAGetLastError();
				NOX_ERROR_LINE(nox::dev::net::log_id::DevNet, nox::util::Format(u"select(handshake1) failed. error_code={0}", err));
				break;
			}

			if (select_result == 0)
			{
				NOX_ERROR_LINE(nox::dev::net::log_id::DevNet, u"timeout handshake1");
				break;
			}

			if (nox::os::file_descriptor::IsSet(client_socket, except_fds) == true)
			{
				NOX_ERROR_LINE(nox::dev::net::log_id::DevNet, u"exception handshake1");
				break;
			}

			if (nox::os::file_descriptor::IsSet(client_socket, read_fds) == false)
			{
				NOX_ERROR_LINE(nox::dev::net::log_id::DevNet, u"invalid socket handshake1");
				break;
			}

			std::array<char, k_hand_shake_str3.length()> dest_buffer{};
			auto r = this->Receive(client_socket, dest_buffer.data(), static_cast<nox::int32>(k_hand_shake_str3.length()));
			if (!r)
			{
				NOX_ERROR_LINE(nox::dev::net::log_id::DevNet, u"ハンドシェイク3の受信に失敗しました");
				break;
			}

			if (std::string_view(dest_buffer.data(), dest_buffer.size()) != k_hand_shake_str3)
			{
				NOX_ERROR_LINE(nox::dev::net::log_id::DevNet, u"ハンドシェイク3の内容が不正です");
				break;
			}

			//	接続確立
			success = true;


			ConnectionContext connection_context;
			{
				connection_context.socket = client_socket;
				connection_context.port = ::ntohs(addr.sin_port);

				nox::memory::Copy(connection_context.address.data(), &addr.sin_addr, sizeof(addr.sin_addr));

				// IPアドレスの文字列表現を生成
				{
					nox::char_t ip_str_buffer[INET_ADDRSTRLEN] = {};
					::inet_ntop(AF_INET, &addr.sin_addr, ip_str_buffer, sizeof(ip_str_buffer));
					connection_context.peername = nox::util::CharCast<nox::char8>(ip_str_buffer);
				}

				// ポート番号の文字列表現を生成
				{
					nox::char_t port_str_buffer[6] = {};
					::snprintf(port_str_buffer, sizeof(port_str_buffer), "%u", connection_context.port);
					connection_context.portname = nox::util::CharCast<nox::char8>(port_str_buffer);
				}

				connection_context.ip_family = IpFamily::IPv4;
			}
			client_list_.emplace_back(PeerContext{ .socket = client_socket });
			NOX_INFO_LINE(nox::dev::net::log_id::DevNet, u"接続完了 name:{0}, port{1}", connection_context.peername, connection_context.portname);

			OnConnected(connection_context);
		}
	}
}

void nox::dev::net::Server::Update()
{
	if (IsStartup() == false)
	{
		return;
	}

	if (client_list_.empty() == true)
	{
		return;
	}

	::fd_set read_fds;
	nox::os::file_descriptor::Zero(read_fds);

	for (const auto& peer : client_list_)
	{
		nox::os::file_descriptor::Set(peer.socket, read_fds);
	}

	constexpr ::timeval timeout{ .tv_sec = 0, .tv_usec = 0 };
	const auto result = ::select(0, &read_fds, nullptr, nullptr, &timeout);
	if (result <= 0)
	{
		return;
	}

	for (auto it = client_list_.begin(); it != client_list_.end(); )
	{
		if (nox::os::file_descriptor::IsSet(it->socket, read_fds) == false)
		{
			++it;
			continue;
		}

		nox::char_t peek;
		const nox::int32 n = nox::dev::net::Receive(it->socket, &peek, sizeof(peek), MSG_PEEK);

		if (n == 0)
		{
			Disconnect(it->socket);
			it = client_list_.begin();
			continue;
		}

		if (n < 0 && ::WSAGetLastError() != WSAEWOULDBLOCK)
		{
			const auto s = it->socket;
			Disconnect(s);
			it = client_list_.begin();
			continue;
		}

		OnReceive();
		++it;
	}
}

void nox::dev::net::Server::Shutdown()
{
	if (is_startup_ == false)
	{
		return;
	}

	//	全てのクライアントを切断
	while (client_list_.empty() == false)
	{
		this->Disconnect(client_list_.back().socket);
	}

	nox::dev::net::Shutdown(socket_, SD_BOTH);
	nox::dev::net::CloseSocket(socket_);
	socket_ = nox::dev::net::k_raw_invalid_socket;

	nox::dev::net::SocketScheduler::Instance().UnregisterEntity(*this);
}

void nox::dev::net::Server::Connected(const nox::dev::net::ConnectionContext& context)
{
	nox::dev::net::PeerContext peer_context;
	peer_context.connection = context;
	peer_context.socket = k_raw_invalid_socket;
	client_list_.emplace_back(peer_context);

	OnConnected(context);
}

void nox::dev::net::Server::Disconnected(const nox::dev::net::ConnectionContext& context)
{

}

void nox::dev::net::Server::Disconnect(const nox::dev::net::raw_socket_t socket)
{
	nox::dev::net::Shutdown(socket, SD_BOTH);
	nox::dev::net::CloseSocket(socket);

	NOX_LOCAL_SCOPE(nox::util::ParallelExecuteCheckScope(pe_checker_clients_));

	auto it = std::find_if(client_list_.begin(), client_list_.end(), [socket](const PeerContext& context) {
		return context.socket == socket;
		});

	if (it != client_list_.end())
	{
		NOX_INFO_LINE(nox::dev::net::log_id::DevNet, u"切断完了 name:{0}, port:{1}", it->connection.peername, it->connection.portname);
		OnDisconnected(it->connection);
		client_list_.erase(it);
	}
	else
	{
		NOX_ASSERT(false, u8"切断するクライアントが見つかりませんでした");
	}
}