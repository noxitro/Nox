//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	network.h
///	@brief	OS のソケット API を包む薄い層
///	@details	OS ごとの違い (ソケットの型・フラグの値・エラーの取り方・sockaddr の構造) はここに出さず、
///				各 OS の実装 (detail/network_<os>.cpp) の中で変換する。
///				ヒープ確保はしない。エラーは戻り値の Error で返す。
#pragma once
#include	<span>
#include	<string_view>
#include	"basic_type.h"
#include	"basic_definition.h"

namespace nox::os::network
{
	///	@brief	IP アドレスの種類
	enum class IpFamily : nox::uint8
	{
		Unknown = 0,
		IPv4 = 1,
		IPv6 = 2
	};

	///	@brief	トランスポートの種類
	enum class Protocol : nox::uint8
	{
		Tcp,
		Udp,
	};

	///	@brief	Shutdown で止める方向
	enum class ShutdownMode : nox::uint8
	{
		Receive,
		Send,
		Both,
	};

	///	@brief	送信のフラグ。値は nox 独自のビットで、OS の定数へは実装側で変換する
	///	@details	POSIX では切れた相手への送信で SIGPIPE が出ないよう、実装側が常に抑止する。
	enum class SendFlag : nox::uint32
	{
		None = 0,
		Oob = 1u << 0,
		DontRoute = 1u << 1,
	};

	///	@brief	受信のフラグ。値は nox 独自のビットで、OS の定数へは実装側で変換する
	enum class ReceiveFlag : nox::uint32
	{
		None = 0,
		Peek = 1u << 0,
		Oob = 1u << 1,
		WaitAll = 1u << 2,
	};

	///	@brief	Poll で待つ / 返ってくる事象
	enum class PollEvent : nox::uint8
	{
		None = 0,
		Readable = 1u << 0,
		Writable = 1u << 1,
		///	@brief	返り値だけに立つ
		Error = 1u << 2,
		///	@brief	返り値だけに立つ
		HangUp = 1u << 3,
		///	@brief	返り値だけに立つ。無効なソケットを渡した
		Invalid = 1u << 4,
	};

	inline constexpr SendFlag operator|(const SendFlag lhs, const SendFlag rhs) noexcept
	{
		return static_cast<SendFlag>(static_cast<nox::uint32>(lhs) | static_cast<nox::uint32>(rhs));
	}

	inline constexpr ReceiveFlag operator|(const ReceiveFlag lhs, const ReceiveFlag rhs) noexcept
	{
		return static_cast<ReceiveFlag>(static_cast<nox::uint32>(lhs) | static_cast<nox::uint32>(rhs));
	}

	inline constexpr PollEvent operator|(const PollEvent lhs, const PollEvent rhs) noexcept
	{
		return static_cast<PollEvent>(static_cast<nox::uint8>(lhs) | static_cast<nox::uint8>(rhs));
	}

	inline constexpr PollEvent operator&(const PollEvent lhs, const PollEvent rhs) noexcept
	{
		return static_cast<PollEvent>(static_cast<nox::uint8>(lhs) & static_cast<nox::uint8>(rhs));
	}

	///	@brief	flag のどれかが立っているか
	inline constexpr bool HasAny(const PollEvent value, const PollEvent flag) noexcept
	{
		return (value & flag) != PollEvent::None;
	}

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

	///	@brief	ログ向けの名前
	const char16_t* ToString(Error error) noexcept;

	using port_t = nox::uint16;

	///	@brief	ソケットのハンドル
	///	@details	Windows の SOCKET (UINT_PTR、無効値は ~0) と POSIX の int (無効値は -1) は、
	///				どちらも intptr に入れると無効値が -1 になる。
	struct Socket
	{
		static constexpr nox::intptr kInvalidValue = -1;

		nox::intptr value = kInvalidValue;

		inline constexpr bool IsValid() const noexcept { return value != kInvalidValue; }
		friend constexpr bool operator==(const Socket&, const Socket&) noexcept = default;
	};

	///	@brief	IP アドレスとポート
	///	@details	sockaddr は OS で構造が違う (BSD 系は先頭に長さを持つ) ので公開しない。
	///				address はネットワークバイト順 (IPv4 は先頭 4 バイト)、port はホストのバイト順。
	struct EndPoint
	{
		IpFamily family = IpFamily::Unknown;
		port_t port = 0;
		///	@brief	IPv6 のスコープ ID (リンクローカル用)
		nox::uint32 scope_id = 0;
		nox::uint8 address[16] = {};

		friend constexpr bool operator==(const EndPoint&, const EndPoint&) noexcept = default;

		static inline constexpr EndPoint MakeIPv4(const nox::uint8 a, const nox::uint8 b, const nox::uint8 c, const nox::uint8 d, const port_t port) noexcept
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
		static inline constexpr EndPoint IPv4Any(const port_t port) noexcept { return MakeIPv4(0, 0, 0, 0, port); }

		///	@brief	127.0.0.1
		static inline constexpr EndPoint IPv4Loopback(const port_t port) noexcept { return MakeIPv4(127, 0, 0, 1, port); }

		///	@brief	::
		static inline constexpr EndPoint IPv6Any(const port_t port) noexcept
		{
			EndPoint result;
			result.family = IpFamily::IPv6;
			result.port = port;
			return result;
		}

		///	@brief	::1
		static inline constexpr EndPoint IPv6Loopback(const port_t port) noexcept
		{
			EndPoint result = IPv6Any(port);
			result.address[15] = 1;
			return result;
		}
	};

	///	@brief	送受信の結果
	///	@details	TCP の Receive で bytes == 0 かつ error == None は、相手が送信を閉じたことを表す。
	struct IoResult
	{
		nox::int32 bytes = 0;
		Error error = Error::None;
	};

	///	@brief	Poll に渡す 1 件
	struct PollEntry
	{
		Socket socket;
		///	@brief	待つ事象 (Readable / Writable)
		PollEvent requested = PollEvent::None;
		///	@brief	起きた事象。Poll が書き込む
		PollEvent returned = PollEvent::None;
	};

	///	@brief	1 回の Poll で渡せる上限。OS の構造体への変換をスタック上で行うため
	inline constexpr nox::int32 kMaxPollEntries = 64;

	///	@brief	ソケット API を使う前に 1 回呼ぶ (Windows は WSAStartup。他の OS では何もしない)
	///	@details	Finalize と対で、入れ子に呼んでよい。
	Error Initialize() noexcept;

	///	@brief	Initialize と対で呼ぶ
	void Finalize() noexcept;

	///	@brief	ソケットを作る
	///	@details	子プロセスへは継承させない。POSIX で SO_NOSIGPIPE がある OS (macOS など) では、ここで設定する。
	Error CreateSocket(IpFamily family, Protocol protocol, Socket& out_socket) noexcept;

	///	@brief	ソケットを閉じ、socket を無効値にする。無効なソケットには何もしない
	Error Close(Socket& socket) noexcept;

	Error Bind(Socket socket, const EndPoint& end_point) noexcept;
	Error Listen(Socket socket, nox::int32 backlog) noexcept;

	///	@param	out_remote	相手のアドレス。不要なら nullptr
	Error Accept(Socket listener, Socket& out_socket, EndPoint* out_remote) noexcept;

	///	@brief	接続する
	///	@details	ノンブロッキングでは InProgress を返す。Poll で Writable を待ち、GetPendingError で結果を確かめる。
	Error Connect(Socket socket, const EndPoint& end_point) noexcept;

	///	@brief	溜まっているエラー (SO_ERROR) を取り出す。ノンブロッキングの Connect の結果を確かめる用
	Error GetPendingError(Socket socket, Error& out_error) noexcept;

	Error Shutdown(Socket socket, ShutdownMode mode) noexcept;

	IoResult Send(Socket socket, std::span<const nox::uint8> buffer, SendFlag flags = SendFlag::None) noexcept;
	IoResult Receive(Socket socket, std::span<nox::uint8> buffer, ReceiveFlag flags = ReceiveFlag::None) noexcept;
	IoResult SendTo(Socket socket, std::span<const nox::uint8> buffer, const EndPoint& destination, SendFlag flags = SendFlag::None) noexcept;

	///	@param	out_source	送信元のアドレス。不要なら nullptr
	IoResult ReceiveFrom(Socket socket, std::span<nox::uint8> buffer, EndPoint* out_source, ReceiveFlag flags = ReceiveFlag::None) noexcept;

	///	@brief	ソケットを待つ
	///	@param	entries	kMaxPollEntries 件まで
	///	@param	timeout_milliseconds	負なら無期限に待つ
	///	@param	out_ready_count	事象が起きた件数。タイムアウトなら 0
	Error Poll(std::span<PollEntry> entries, nox::int32 timeout_milliseconds, nox::int32& out_ready_count) noexcept;

	Error SetNonBlocking(Socket socket, bool enable) noexcept;

	///	@brief	TCP_NODELAY (Nagle を止める)
	Error SetNoDelay(Socket socket, bool enable) noexcept;

	///	@brief	IPV6_V6ONLY。既定値は OS で違う (Windows は有効、Linux は無効) ので、IPv6 のソケットは明示して揃える
	Error SetIPv6Only(Socket socket, bool enable) noexcept;

	Error SetSendBufferSize(Socket socket, nox::int32 size) noexcept;
	Error SetReceiveBufferSize(Socket socket, nox::int32 size) noexcept;

	///	@brief	ソケットに割り当てられたアドレス (ポート 0 で Bind したときの実際のポートなど)
	Error GetLocalEndPoint(Socket socket, EndPoint& out_end_point) noexcept;
	Error GetRemoteEndPoint(Socket socket, EndPoint& out_end_point) noexcept;

	///	@brief	数値表記のアドレス ("127.0.0.1"、"::1" など) を読む。名前解決はしない
	///	@return	読めなければ InvalidArgument
	Error ParseAddress(std::string_view text, port_t port, EndPoint& out_end_point) noexcept;
}
