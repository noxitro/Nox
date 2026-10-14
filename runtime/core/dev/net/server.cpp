//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	server.cpp
///	@brief	server
#include	"pch.h"
#include	"server.h"

#include	"socket_scheduler.h"
#include	"dev_net_log_id.h"

namespace nox::dev::net
{
	inline constexpr std::string_view k_hand_shake_str1 = "HandShake1";
	inline constexpr std::string_view k_hand_shake_str2 = "HandShake2";
	inline constexpr std::string_view k_hand_shake_str3 = "HandShake3";

	/// @brief ハンドシェイクで相手の送信を待つ時間
	inline constexpr nox::int32 k_hand_shake_timeout_milliseconds = 5000;

	namespace
	{
		inline nox::int32 ToLogCode(const nox::os::network::Error error) noexcept
		{
			return static_cast<nox::int32>(nox::util::ToUnderlying(error));
		}

		/// @brief ハンドシェイクの受信を待つ
		/// @param stage ログに出すハンドシェイクの段階 (1 か 3)
		/// @return 読めるようになったら true。タイムアウト・エラーならログを出して false
		bool WaitHandShake(const nox::os::network::Socket socket, const nox::int32 stage)
		{
			std::array<nox::os::network::PollEntry, 1> entries{};
			entries[0].socket = socket;
			entries[0].requested = nox::os::network::PollEvent::Readable;

			const auto poll_result = nox::os::network::Poll(entries, k_hand_shake_timeout_milliseconds);
			if (poll_result.has_value() == false)
			{
				NOX_ERROR_LINE(nox::dev::net::log_id::DevNet, u"poll(handshake{0}) failed. error={1}", stage, ToLogCode(poll_result.error()));
				return false;
			}

			if (*poll_result == 0)
			{
				NOX_ERROR_LINE(nox::dev::net::log_id::DevNet, u"timeout handshake{0}", stage);
				return false;
			}

			const nox::os::network::PollEvent returned = entries[0].returned;
			if (nox::util::IsBitAnd(returned, nox::os::network::PollEvent::Error) ||
				nox::util::IsBitAnd(returned, nox::os::network::PollEvent::Invalid))
			{
				NOX_ERROR_LINE(nox::dev::net::log_id::DevNet, u"exception handshake{0}", stage);
				return false;
			}

			//	相手が閉じたとき (HangUp) も Readable が立ち、続く受信が 0 バイトで切断を返す
			if (nox::util::IsBitAnd(returned, nox::os::network::PollEvent::Readable) == false)
			{
				NOX_ERROR_LINE(nox::dev::net::log_id::DevNet, u"invalid socket handshake{0}", stage);
				return false;
			}
			return true;
		}
	}
}

nox::dev::net::Server::Server(nox::dev::net::IServerEventHandler& event_handler)noexcept :
	event_handler_(event_handler),
	initialize_context_{},
	socket_{},
	is_startup_(false)
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
	initialize_context_ = context;

	const auto created = nox::os::network::CreateSocket(nox::os::network::IpFamily::IPv4, nox::os::network::Protocol::TCP);
	if (created.has_value() == false)
	{
		NOX_ERROR_LINE(nox::dev::net::log_id::DevNet, u"socketの作成に失敗しました error={0}", ToLogCode(created.error()));
		return false;
	}
	this->socket_ = *created;

	//	失敗したら作ったソケットを閉じる
	bool success = false;
	NOX_LOCAL_SCOPE(nox::util::ScopeExit([this, &success]() {
		if (!success)
		{
			(void)nox::os::network::Close(this->socket_);
		}
		}));

	//	非ブロッキング
	if (const nox::os::network::Error error = nox::os::network::SetNonBlocking(this->socket_, true); error != nox::os::network::Error::None)
	{
		NOX_ERROR_LINE(nox::dev::net::log_id::DevNet, u"非ブロッキングの設定に失敗しました error={0}", ToLogCode(error));
		return false;
	}

	//	遅延を減らす
	(void)nox::os::network::SetNoDelay(this->socket_, true);

	//	終了直後に立ち上げ直しても bind できるようにする
	(void)nox::os::network::SetReuseAddress(this->socket_, true);

	//TODO:	ネットワークレイテンシの向上

	//	socketにアドレスを割り当て (全てのマシンを受け付け)
	if (const nox::os::network::Error error = nox::os::network::Bind(this->socket_, nox::os::network::EndPoint::IPv4Any(this->initialize_context_.port)); error != nox::os::network::Error::None)
	{
		NOX_ERROR_LINE(nox::dev::net::log_id::DevNet, u"Bindに失敗しました port={0} error={1}", this->initialize_context_.port, ToLogCode(error));
		return false;
	}

	//	接続待ち状態にする
	if (nox::os::network::Listen(this->socket_, this->initialize_context_.max_connection) != nox::os::network::Error::None)
	{
		NOX_ERROR_LINE(nox::dev::net::log_id::DevNet, u"Listenに失敗しました");
		return false;
	}

	success = true;
	is_startup_ = true;
	return true;
}

void	nox::dev::net::Server::Connection()
{
	if (IsStartup() == false)
	{
		return;
	}

	while (true)
	{
		//	受け付けられる接続が無くなるまでacceptを繰り返す
		auto accepted = nox::os::network::Accept(this->socket_, nox::os::network::SocketMode::NonBlocking);
		if (accepted.has_value() == false)
		{
			const nox::os::network::Error error = accepted.error();
			if (error == nox::os::network::Error::WouldBlock || error == nox::os::network::Error::Interrupted)
			{
				//	接続待ち無し
				break;
			}
			NOX_ERROR_LINE(nox::dev::net::log_id::DevNet, u"Acceptに失敗しました。error={0}", ToLogCode(error));
			break;
		}

		nox::os::network::Socket client_socket = accepted->socket;

		// 接続ソケットに低遅延/KeepAliveを適用
		(void)nox::os::network::SetNoDelay(client_socket, true);
		(void)nox::os::network::SetKeepAlive(client_socket, true);

		bool success = false;
		NOX_LOCAL_SCOPE(nox::util::ScopeExit([&success, &client_socket]() {
			if (!success)
			{
				(void)nox::os::network::Close(client_socket);
			}
			}));

		//	handshake1の受信
		{
			if (WaitHandShake(client_socket, 1) == false)
			{
				break;
			}

			std::array<char, k_hand_shake_str1.length()> dest_buffer{0};
			auto r = nox::dev::net::ReceiveAll(client_socket, dest_buffer.data(), static_cast<nox::int32>(k_hand_shake_str1.length()));
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
			const auto r = nox::dev::net::SendAll(client_socket, k_hand_shake_str2.data(), static_cast<nox::int32>(k_hand_shake_str2.length()));
			if (!r)
			{
				NOX_ERROR_LINE(nox::dev::net::log_id::DevNet, u"ハンドシェイク2の送信に失敗しました");
				break;
			}
		}

		//	handshake3の受信
		{
			if (WaitHandShake(client_socket, 3) == false)
			{
				break;
			}

			std::array<char, k_hand_shake_str3.length()> dest_buffer{};
			auto r = nox::dev::net::ReceiveAll(client_socket, dest_buffer.data(), static_cast<nox::int32>(k_hand_shake_str3.length()));
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
				connection_context.end_point = accepted->remote;

				// IPアドレスの文字列表現を生成
				{
					std::array<char, nox::os::network::kMaxAddressStringLength> ip_str_buffer{};
					if (nox::os::network::FormatAddress(accepted->remote, ip_str_buffer).has_value())
					{
						connection_context.peername = nox::util::CharCast<nox::char8>(ip_str_buffer.data());
					}
				}

				// ポート番号の文字列表現を生成
				{
					nox::char_t port_str_buffer[6] = {};
					::snprintf(port_str_buffer, sizeof(port_str_buffer), "%u", static_cast<unsigned int>(accepted->remote.port));
					connection_context.portname = nox::util::CharCast<nox::char8>(port_str_buffer);
				}
			}
          client_list_.emplace_back(PeerContext{ .connection = connection_context, .socket = client_socket });
			NOX_INFO_LINE(nox::dev::net::log_id::DevNet, u"接続完了 name:{0}, port{1}", connection_context.peername, connection_context.portname);

			event_handler_.OnServerConnected(connection_context);
		}
	}
}

void nox::dev::net::Server::Update(nox::legacy::World& world)
{
	if (IsStartup() == false)
	{
		return;
	}

	if (client_list_.empty() == true)
	{
		return;
	}

	//	読めるクライアントを集める。ヒープを使わないよう、kMaxPollEntries 件ずつスタック上で調べる
	std::array<nox::os::network::PollEntry, nox::os::network::kMaxPollEntries> entries{};
	std::array<nox::os::network::Socket, nox::os::network::kMaxPollEntries> readable_sockets{};
	for (std::size_t begin = 0; begin < client_list_.size(); begin += entries.size())
	{
		const std::size_t count = std::min(entries.size(), client_list_.size() - begin);
		for (std::size_t i = 0; i < count; ++i)
		{
			entries[i].socket = client_list_[begin + i].socket;
			entries[i].requested = nox::os::network::PollEvent::Readable;
		}

		const auto poll_result = nox::os::network::Poll(std::span(entries.data(), count), 0);
		if (poll_result.has_value() == false || *poll_result <= 0)
		{
			continue;
		}

		//	Disconnect で client_list_ が変わるので、先に読めるソケットを写しておく
		std::size_t readable_count = 0;
		for (std::size_t i = 0; i < count; ++i)
		{
			if (entries[i].returned != nox::os::network::PollEvent::None)
			{
				readable_sockets[readable_count++] = entries[i].socket;
			}
		}

		for (std::size_t i = 0; i < readable_count; ++i)
		{
			const nox::os::network::Socket socket = readable_sockets[i];
			const auto current_it = std::find_if(client_list_.begin(), client_list_.end(), [socket](const PeerContext& context) {
				return context.socket == socket;
				});
			if (current_it == client_list_.end())
			{
				continue;
			}

			nox::uint8 peek = 0;
			const nox::os::network::IoResult result = nox::os::network::Receive(socket, std::span(&peek, 1), nox::os::network::ReceiveFlag::Peek);

			if (result.IsOk() && result.bytes == 0)
			{
				Disconnect(socket);
				continue;
			}

			if (result.IsOk() == false && result.error != nox::os::network::Error::WouldBlock)
			{
				Disconnect(socket);
				continue;
			}

			event_handler_.OnServerReceive(world);
		}

		//	Disconnect で数が減ったときは、続きの位置がずれるので次の Update に回す
		if (begin + count > client_list_.size())
		{
			break;
		}
	}
}

void nox::dev::net::Server::Shutdown()
{
	if (is_startup_ == false)
	{
		return;
	}

	is_startup_ = false;

	//	全てのクライアントを切断
	while (client_list_.empty() == false)
	{
		this->Disconnect(client_list_.back().socket);
	}

	(void)nox::os::network::Shutdown(socket_, nox::os::network::ShutdownMode::Both);
	(void)nox::os::network::Close(socket_);
}

void nox::dev::net::Server::Disconnect(const nox::os::network::Socket socket)
{
#if !NOX_MASTER
	NOX_LOCAL_SCOPE(nox::util::ParallelExecuteCheckScope(pe_checker_clients_));
#endif // !NOX_MASTER

	auto it = std::find_if(client_list_.begin(), client_list_.end(), [socket](const PeerContext& context) {
		return context.socket == socket;
		});

	if (it == client_list_.end())
	{
		NOX_WARNING_LINE(nox::dev::net::log_id::DevNet, u8"切断するクライアントが見つかりませんでした socket={0}", socket.value);
		return;
	}

	nox::os::network::Socket closing = socket;
	(void)nox::os::network::Shutdown(closing, nox::os::network::ShutdownMode::Both);
	(void)nox::os::network::Close(closing);

	NOX_INFO_LINE(nox::dev::net::log_id::DevNet, u"切断完了 name:{0}, port:{1}", it->connection.peername, it->connection.portname);
	event_handler_.OnServerDisconnected(it->connection);
	client_list_.erase(it);
}

std::expected<void, nox::dev::net::SocketIoError> nox::dev::net::Server::Send(
	nox::os::network::Socket socket,
	nox::not_null<const void*> buffer,
	nox::int32 size_to_send,
	nox::os::network::SendFlag flag)
{
	return nox::dev::net::SendAll(socket, buffer, size_to_send, flag);
}
