//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	dev_net_definition.h
///	@brief	dev_net_definition
///	@details	ソケットの操作は kernel の nox::os::network を使う。OS のヘッダーや型はここに持ち込まない。
#pragma once

namespace nox::dev::net
{
	///	@brief	ハンドシェイクの段階 (Editor と取り決めたプロトコル)
	enum class ConnectionState : nox::uint8
	{
		Invalid,
		HandShake1,
		HandShake2,
		HandShake3,
		Connected,
	};

	using Port = nox::os::network::Port;

	struct ConnectionContext
	{
		nox::uint32 unique_id = 0;

		///	@brief	接続元のアドレスとポート
		nox::os::network::EndPoint end_point;

		/// @brief IPアドレスの文字列表現 (例: "192.168.0.1")
		nox::U8FixedString<nox::os::network::kMaxAddressStringLength> peername;
		/// @brief ポート番号の文字列表現 (例: "8080")
		nox::U8FixedString<6> portname;
		nox::os::network::Socket socket;
	};
	using DisconnectionContext = ConnectionContext;

	struct PeerContext
	{
		ConnectionContext connection;
		nox::os::network::Socket socket;
	};

	struct SocketIoError
	{
		enum class Kind : nox::uint8
		{
			InvalidSocket,   // 無効なソケット
			Disconnected,    // 正常切断/相手切断
			WouldBlock,      // 非ブロッキングで未完了
			Interrupted,     // 割り込み
			Other,           // 上記以外（network_error を参照）
		};
		using enum Kind;

		Kind kind;

		/// @brief ソケット層のエラー (未設定は None)
		nox::os::network::Error network_error = nox::os::network::Error::None;
	};
}
