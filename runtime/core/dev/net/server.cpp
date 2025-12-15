//	Copyright (c) 2025 NOX ENGINE All rights reserved.

///	@file	server.cpp
///	@brief	server
#include	"stdafx.h"
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

nox::dev::net::Server::~Server()
{
	nox::dev::net::SocketScheduler::Instance().UnregisterEntity(*this);
}

inline	void	nox::dev::net::Server::SetFlag(Flag flag, bool is_on)noexcept
{
	if (is_on)
	{
		flags_ = static_cast<Flag>(static_cast<nox::uint8>(flags_) | static_cast<nox::uint8>(flag));
	}
	else
	{
		flags_ = static_cast<Flag>(static_cast<nox::uint8>(flags_) & ~static_cast<nox::uint8>(flag));
	}
}

inline	bool	nox::dev::net::Server::IsFlag(Flag flag)const noexcept
{
	return (static_cast<nox::uint8>(flags_) & static_cast<nox::uint8>(flag)) != 0;
}

bool nox::dev::net::Server::Startup(const InitializeContext& context)
{
	if (IsFlag(Flag::Startup))
	{
		return false;
	}

	initialize_context_ = context;
	shutdown_ = false;
	phase_ = Phase::None;
	is_error_ = false;

#if NOX_WINDOWS
	raw_sockaddr_in ip_address;
	ip_address.sin_family = AF_INET;
	ip_address.sin_addr.S_un.S_addr = INADDR_ANY;
	ip_address.sin_port = ::htons(static_cast<nox::uint16>(context.port));
	this->socket_ = ::socket(ip_address.sin_family, SOCK_STREAM, 0);
	
	NOX_ASSERT(socket_ != INVALID_SOCKET, nox::util::Format(u"socket() failed. error_code={0}", ::WSAGetLastError()));

	//	非ブロッキング
	::u_long nb = 1;
	if(::ioctlsocket(this->socket_, FIONBIO, &nb) == k_raw_error_socket)
	{
		NOX_ASSERT(false, nox::util::Format(u"ioctlsocket() failed. error_code={0}", ::WSAGetLastError()));
	}

	//	遅延を減らす
	nox::dev::net::SetNoDelay(this->socket_, true);
#endif // NOX_WINDOWS

	//	socketにアドレスを割り当て
	nox::dev::net::Bind(this->socket_, *reinterpret_cast<const nox::dev::net::raw_sockaddr*>(&ip_address), sizeof(ip_address));

	//	接続町状態にする
	if (nox::dev::net::Listen(this->socket_, context.max_connection) == k_raw_error_socket)
	{
		NOX_ERROR_LINE(nox::dev::net::log_id::DevNet, u"Listenに失敗しました");
		return false;
	}

	SetFlag(Flag::Startup, true);
	ChangePhase(Phase::Startuped);

	nox::dev::net::SocketScheduler::Instance().RegisterEntity(*this);
	return true;
}

void nox::dev::net::Server::Update()
{
	switch (phase_)
	{
	case Phase::None:
		break;

	case Phase::Startuped:
		PollAccept();
		break;
	}
}

inline	void nox::dev::net::Server::ChangePhase(nox::dev::net::Server::Phase phase)
{
	NOX_ASSERT(phase_ != phase, u"Same phase.");

	switch (phase)
	{
	case Phase::Startuped:
		break;
	}

	phase_ = phase;
}

void nox::dev::net::Server::PollAccept()
{
	if (IsStartup() == false)
	{
		return;
	}

	nox::dev::net::SocketScheduler& socket_scheduler = nox::dev::net::SocketScheduler::Instance();

	while (true)
	{
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

		bool success = false;
		nox::util::ScopeExit([&success, &client_socket]()
			{
				if (!success)
				{
					nox::dev::net::CloseSocket(client_socket);
				}
			});

		// 接続ソケットに低遅延/KeepAlive等の推奨オプションを適用（任意）
		{
			::u_long nb = 1;
			::ioctlsocket(client_socket, FIONBIO, &nb);
			int on = 1;

			nox::dev::net::SetNoDelay(client_socket, true);

			::setsockopt(client_socket, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char*>(&on), sizeof(on));
			::setsockopt(client_socket, SOL_SOCKET, SO_KEEPALIVE, reinterpret_cast<const char*>(&on), sizeof(on));
		}

		//	handshakeを受信
		{
			std::array<char, k_hand_shake_str1.length()> dest_buffer{};
			auto r = this->Receive(client_socket, dest_buffer.data(), static_cast<nox::int32>(k_hand_shake_str1.length()));
			if (!r)
			{
				NOX_ERROR_LINE(nox::dev::net::log_id::DevNet, u"ハンドシェイク1の受信に失敗しました");
				break;
			}
			if (dest_buffer.data() != k_hand_shake_str1)
			{
				NOX_ERROR_LINE(nox::dev::net::log_id::DevNet, u"ハンドシェイク1の内容が不正です");
				break;
			}
		}

		//	handshakeを送信
		{
			auto r = this->Send(client_socket, k_hand_shake_str2.data(), static_cast<nox::int32>(k_hand_shake_str2.length()));
			if (!r)
			{
				NOX_ERROR_LINE(nox::dev::net::log_id::DevNet, u"ハンドシェイク2の送信に失敗しました");
				break;
			}
		}

		//	handshakeを受信
		{
			std::array<char, k_hand_shake_str3.length()> dest_buffer{};
			auto r = this->Receive(client_socket, dest_buffer.data(), static_cast<nox::int32>(k_hand_shake_str3.length()));
			if (!r)
			{
				NOX_ERROR_LINE(nox::dev::net::log_id::DevNet, u"ハンドシェイク3の受信に失敗しました");
				break;
			}
			if (dest_buffer.data() != k_hand_shake_str3)
			{
				NOX_ERROR_LINE(nox::dev::net::log_id::DevNet, u"ハンドシェイク1の内容が不正です");
				break;
			}
		}

		success = true;

		nox::dev::net::ConnectionContext context;
		context.unique_id = 0; // TODO: ユニークIDの発行
		context.address = *reinterpret_cast<nox::dev::net::address_t*>(&addr.sin_addr);
		context.port = ::ntohs(addr.sin_port);

		Connected(context);
	}
}

void nox::dev::net::Server::Shutdown()
{
	nox::dev::net::Shutdown(socket_, SD_BOTH);
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