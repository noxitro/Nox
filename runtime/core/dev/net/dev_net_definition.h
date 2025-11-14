//	Copyright (C) 2025 NOX ENGINE All rights reserved.

///	@file	dev_net_definition.h
///	@brief	dev_net_definition
#pragma once

namespace nox::dev::net
{
	enum class ConnectionState : nox::uint8
	{
		Invalid,
		HandShake1,
		HandShake2,
		HandShake3,
		Connected,
	};

	// --- 新規: IP アドレス / エンドポイント抽象 ------------------------
	enum class IpFamily : nox::uint8 
	{ 
		Unknown = 0, 
		IPv4 = 1, 
		IPv6 = 2 
	};

	/// @brief 受信フラグ
	enum class ReceiveFlag : nox::int32
	{
		/// @brief 通常受信
		None = 0,

#if NOX_WINDOWS
		/// @brief バッファから取り出さずに先読み（ヘッダ検査などに有用）
		Peek = MSG_PEEK,
		/// @brief 緊急データ（ほぼ未使用）
		Oob = MSG_OOB,
		/// @brief 指定バイト数に達するまでブロック（非ブロッキングソケットでは非推奨）
		WaitAll = MSG_WAITALL,
#endif // NOX_WINDOWS
	};

	/// @brief 送信フラグ
	enum class SendFlag : nox::int32
	{
		/// @brief 通常送信
		None = 0,
#if NOX_WINDOWS
		Oob = MSG_OOB,
		DontRoute = MSG_DONTROUTE,
		Partial = MSG_PARTIAL, // メッセージ指向のみ想定。TCPでは通常使わない
#endif
	};

#if NOX_WINDOWS
	using port_t = nox::uint32;
	using raw_socket_t = ::SOCKET;
	using raw_sockaddr = ::sockaddr;
	using raw_sockaddr_in = ::sockaddr_in;
	constexpr raw_socket_t k_raw_invalid_socket = INVALID_SOCKET;
	constexpr nox::int32 k_raw_error_socket = SOCKET_ERROR;

	struct address_t
	{
		constexpr nox::uint8* data() noexcept { return buffer; }
		inline constexpr const nox::uint8* data()const noexcept { return buffer; }

	private:
		nox::uint8 buffer[64];
	};

	struct peer_name_t
	{
		constexpr nox::uint8* data() noexcept { return buffer; }
		inline constexpr const nox::uint8* data()const noexcept { return buffer; }

	private:
		nox::uint8 buffer[64];
	};

#endif // NOX_WINDOWS
	using port_name_t = peer_name_t;

	struct ConnectionContext
	{
		nox::uint32 unique_id;
		address_t address;
		port_t port;
		peer_name_t peername;
		port_name_t portname;
	};
	using DisconnectionContext = ConnectionContext;

	struct PeerContext
	{
		ConnectionContext connection;
		raw_socket_t socket;
	};

	struct SocketIoError
	{
		enum class Kind : nox::uint8
		{
			InvalidSocket,   // 無効なソケット
			Disconnected,    // 正常切断/相手切断
			WouldBlock,      // 非ブロッキングで未完了（WSAEWOULDBLOCK）
			Interrupted,     // 割り込み（WSAEINTR）
			Other,           // 上記以外（platform_code を参照）
		};
		using enum Kind;

		Kind kind;

		/// @brief プラットフォーム固有のエラーコード（Windows: WSAGetLastError() の値、未設定=0）
		nox::int32 platform_code=0;
	};
}