//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	network.h
///	@brief	network
#pragma once
#include	<expected>
#include	<span>
#include	<string_view>
#include	<type_traits>
#include	"basic_type.h"
#include	"basic_definition.h"

namespace nox::os::network
{
	/// @brief IPアドレスの種類
	enum class IpFamily : nox::uint8
	{
		Unknown,
		IPv4,
		IPv6
	};

	/// @brief トランスポートプロトコルの種類
	enum class Protocol : nox::uint8
	{
		TCP,
		UDP,
	};

	/// @brief Shutdownで止める方向
	enum class ShutdownMode : nox::uint8
	{
		Receive,
		Send,
		Both,
	};

	enum class ReceiveFlag : nox::uint32
	{
		None = 0,
		Peek = 1 << 0,
		Oob = 1 << 1,
		WaitAll = 1 << 2,
	};

	enum class SendFlag : nox::uint32
	{
		None = 0,
		Oob = 1 << 0,
		DontRoute = 1 << 1,
	};

	/// @brief pollで待つ/返ってくる事象
	enum class PollEvent : nox::uint8
	{
		None = 0,
		Readable = 1 << 0,
		Writable = 1 << 1,
		Error = 1 << 2,
		HangUp = 1 << 3,
		Invalid = 1 << 4,
	};

	NOX_ENABLE_ENUM_CLASS_FLAG(nox::os::network::SendFlag)
	NOX_ENABLE_ENUM_CLASS_FLAG(nox::os::network::ReceiveFlag)
	NOX_ENABLE_ENUM_CLASS_FLAG(nox::os::network::PollEvent)

	///	@brief	ソケット操作のエラー。OS のエラーコード (WSAGetLastError / errno) から変換する
	enum class Error : nox::uint8
	{
		None = 0,
		///	@brief	ノンブロッキングで、今は完了できない
		WouldBlock,
		///	@brief	ノンブロッキングの Connect が進行中
		InProgress,
		Interrupted,
		TimedOut,
		ConnectionReset,
		ConnectionRefused,
		ConnectionAborted,
		NotConnected,
		AddressInUse,
		AddressNotAvailable,
		NetworkUnreachable,
		HostUnreachable,
		MessageTooLong,
		InvalidArgument,
		NoResources,
		AccessDenied,
		///	@brief	Initialize を呼んでいない (Windows のみ)
		NotInitialized,
		Unknown,
	};

	enum class SocketMode : nox::uint8
	{
		Blocking,
		NonBlocking,
	};

	/// @brief 0~65535 の範囲のポート番号
	using Port = nox::uint16;

	/// @brief		ソケットハンドル
	/// @details	
	struct Socket final
	{
		static constexpr nox::intptr kInvalidSocket = static_cast<nox::intptr>(-1);
		nox::intptr value = kInvalidSocket;
		inline constexpr bool IsValid() const noexcept { return value != kInvalidSocket; }
		constexpr bool operator==(const Socket& rhs) const noexcept = default;
	};

	struct EndPoint final
	{
		nox::os::network::IpFamily family = nox::os::network::IpFamily::Unknown;
		nox::os::network::Port port = 0;
		/// @brief IPv6のスコープID。IPv4では0
		nox::uint32 scope_id = 0;
		std::array<nox::uint8, 16> address = {};

		constexpr bool operator==(const EndPoint& rhs) const noexcept = default;

		static inline constexpr EndPoint MakeIPv4(const nox::uint8 a, const nox::uint8 b, const nox::uint8 c, const nox::uint8 d, const Port port) noexcept
		{
			EndPoint result;
			result.family = IpFamily::IPv4;
			result.port = port;
			result.address[0] = a;
			result.address[1] = b;
			result.address[2] = c;
			result.address[3] = d;
			return result;
		}

		///	@brief	0.0.0.0
		static inline constexpr EndPoint IPv4Any(const Port port) noexcept { return MakeIPv4(0, 0, 0, 0, port); }

		///	@brief	127.0.0.1
		static inline constexpr EndPoint IPv4Loopback(const nox::os::network::Port port) noexcept { return MakeIPv4(127, 0, 0, 1, port); }

		///	@brief	::
		static inline constexpr EndPoint IPv6Any(const nox::os::network::Port port) noexcept
		{
			EndPoint result;
			result.family = IpFamily::IPv6;
			result.port = port;
			return result;
		}

		///	@brief	::1
		static inline constexpr EndPoint IPv6Loopback(const nox::os::network::Port port) noexcept
		{
			EndPoint result = IPv6Any(port);
			result.address[15] = 1;
			return result;
		}
	};

	///	@brief	Accept の結果。相手のアドレスは accept が返すものをそのまま使う
	///	@details	後から getpeername で取ると、相手がすぐ切断していると失敗するため
	struct AcceptedSocket final
	{
		nox::os::network::Socket socket;
		nox::os::network::EndPoint remote;
	};

	///	@brief		送受信の結果
	///	@details	TCP の Receive で bytes == 0 かつ error == None は、相手が送信を閉じたことを表す。
	struct [[nodiscard]] IoResult final
	{
		nox::int32 bytes = 0;
		nox::os::network::Error error = nox::os::network::Error::None;

		inline constexpr bool IsOk() const noexcept { return error == nox::os::network::Error::None; }
	};

	///	@brief	ReceiveFrom の結果。送信元を戻り値に含め、出力の引数を無くす
	struct [[nodiscard]] ReceiveFromResult final
	{
		nox::int32 bytes = 0;
		nox::os::network::Error error = nox::os::network::Error::None;
		nox::os::network::EndPoint source;

		inline constexpr bool IsOk() const noexcept { return error == nox::os::network::Error::None; }
	};

	///	@brief	Poll に渡す 1 件
	struct PollEntry final
	{
		nox::os::network::Socket socket;
		///	@brief	待つ事象 (Readable / Writable)
		nox::os::network::PollEvent requested = nox::os::network::PollEvent::None;
		///	@brief	起きた事象。Poll が書き込む
		nox::os::network::PollEvent returned = nox::os::network::PollEvent::None;
	};

	/// @brief 1回の poll で処理する最大エントリ数
	inline constexpr nox::int32 kMaxPollEntries = 64;

	[[nodiscard]] nox::os::network::Error Initialize()noexcept;
	void Finalize()noexcept;

	/// @brief		ソケット作成
	/// @details	子プロセスへは継承させない
	/// @param family 
	/// @param protocol 
	/// @return 
	[[nodiscard]] std::expected<nox::os::network::Socket, nox::os::network::Error> CreateSocket(nox::os::network::IpFamily family, nox::os::network::Protocol protocol)noexcept;
	///	@param	mode	受けたソケットのモード。待ち受け側の設定を引き継ぐかは OS ごとに違うので、明示して揃える
	[[nodiscard]] std::expected<nox::os::network::AcceptedSocket, nox::os::network::Error> Accept(nox::os::network::Socket listener, nox::os::network::SocketMode mode) noexcept;
	[[nodiscard]] std::expected<nox::os::network::EndPoint, nox::os::network::Error> GetLocalEndPoint(nox::os::network::Socket socket) noexcept;
	[[nodiscard]] std::expected<nox::os::network::EndPoint, nox::os::network::Error> GetRemoteEndPoint(nox::os::network::Socket socket) noexcept;

	///	@brief	数値表記のアドレス ("127.0.0.1"、"::1" など) を読む。名前解決はしない
	[[nodiscard]] std::expected<nox::os::network::EndPoint, nox::os::network::Error> ParseAddress(std::string_view text, nox::os::network::Port port) noexcept;

	///	@param	entries	入出力 (requested を読み、returned を書く)。kMaxPollEntries 件まで
	///	@param	timeout_milliseconds	負なら無期限に待つ
	///	@return	事象が起きた件数。タイムアウトなら 0
	[[nodiscard]] std::expected<nox::int32, nox::os::network::Error> Poll(std::span<nox::os::network::PollEntry> entries, nox::int32 timeout_milliseconds) noexcept;

	///	@brief	ソケットを閉じ、無効値に戻す (入出力の引数)。無効なソケットには何もしない
	[[nodiscard]] nox::os::network::Error Close(nox::os::network::Socket& socket) noexcept;

	[[nodiscard]] nox::os::network::Error Bind(nox::os::network::Socket socket, const nox::os::network::EndPoint& end_point) noexcept;
	[[nodiscard]] nox::os::network::Error Listen(nox::os::network::Socket socket, nox::int32 backlog) noexcept;

	///	@details	ノンブロッキングでは InProgress を返す。Poll でWritable か Error / HangUp を待つ、GetPendingError で結果を見る
	[[nodiscard]] nox::os::network::Error Connect(nox::os::network::Socket socket, const nox::os::network::EndPoint& end_point) noexcept;

	///	@brief	溜まっているエラー (SO_ERROR) を取り出す。取り出し自体の失敗もその Error で返す
	[[nodiscard]] nox::os::network::Error GetPendingError(nox::os::network::Socket socket) noexcept;

	[[nodiscard]] nox::os::network::Error Shutdown(nox::os::network::Socket socket, nox::os::network::ShutdownMode mode) noexcept;
	[[nodiscard]] nox::os::network::Error SetNonBlocking(nox::os::network::Socket socket, bool enable) noexcept;
	[[nodiscard]] nox::os::network::Error SetNoDelay(nox::os::network::Socket socket, bool enable) noexcept;
	[[nodiscard]] nox::os::network::Error SetIPv6Only(nox::os::network::Socket socket, bool enable) noexcept;
	[[nodiscard]] nox::os::network::Error SetSendBufferSize(nox::os::network::Socket socket, nox::int32 size) noexcept;
	[[nodiscard]] nox::os::network::Error SetReceiveBufferSize(nox::os::network::Socket socket, nox::int32 size) noexcept;

	//	---- 送受信 (毎フレーム呼ぶ経路。集成体で返す) ----
	nox::os::network::IoResult Send(nox::os::network::Socket socket, std::span<const nox::uint8> buffer, nox::os::network::SendFlag flags = nox::os::network::SendFlag::None) noexcept;
	nox::os::network::IoResult Receive(nox::os::network::Socket socket, std::span<nox::uint8> buffer, nox::os::network::ReceiveFlag flags = nox::os::network::ReceiveFlag::None) noexcept;
	nox::os::network::IoResult SendTo(nox::os::network::Socket socket, std::span<const nox::uint8> buffer, const nox::os::network::EndPoint& destination, nox::os::network::SendFlag flags = nox::os::network::SendFlag::None) noexcept;
	nox::os::network::ReceiveFromResult ReceiveFrom(nox::os::network::Socket socket, std::span<nox::uint8> buffer, nox::os::network::ReceiveFlag flags = nox::os::network::ReceiveFlag::None) noexcept;
}