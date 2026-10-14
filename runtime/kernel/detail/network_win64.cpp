//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	network_win64.cpp
///	@brief	network_win64
#include	"pch.h"
#include	"../win64_socket.h"
#if NOX_WINDOWS

#include	<cstring>
#include	<optional>
#include	"network_detail.h"
#include	"../algorithm.h"

namespace nox::os::network
{
	namespace
	{
		static_assert(
			static_cast<nox::intptr>(INVALID_SOCKET) == nox::os::network::Socket::kInvalidSocket,
			"INVALID_SOCKETはintptrにすると-1になるはず");

		///	@return	対応しない種類なら 0。AF_UNSPEC (0) は WSASocketW に渡さない
		inline constexpr int GetNativeFamily(nox::os::network::IpFamily family)noexcept
		{
			switch (family)
			{
			case nox::os::network::IpFamily::IPv4: return AF_INET;
			case nox::os::network::IpFamily::IPv6: return AF_INET6;
			default: return 0;
			}
		}

		inline constexpr ::SOCKET ToNative(const nox::os::network::Socket socket)noexcept
		{
			return static_cast<::SOCKET>(socket.value);
		}

		inline constexpr nox::os::network::Socket FromNative(const ::SOCKET socket)noexcept
		{
			return nox::os::network::Socket{ static_cast<nox::intptr>(socket) };
		}

		inline constexpr nox::os::network::Error ToError(const int code)noexcept
		{
			switch (code)
			{
			case 0:
				return nox::os::network::Error::None;
			case WSAEWOULDBLOCK:
				return nox::os::network::Error::WouldBlock;
			//	WSAEINPROGRESS は「Winsock 1.1 のブロッキング呼び出しが進行中」で、接続の進行中 (POSIX の EINPROGRESS) とは別物なので Unknown に落とす
			case WSAEALREADY:
				return nox::os::network::Error::InProgress;
			case WSAEINTR:
				return nox::os::network::Error::Interrupted;
			case WSAETIMEDOUT:
				return nox::os::network::Error::TimedOut;
			case WSAECONNRESET:
			case WSAENETRESET:
				return nox::os::network::Error::ConnectionReset;
			case WSAECONNREFUSED:
				return nox::os::network::Error::ConnectionRefused;
			case WSAECONNABORTED:
				return nox::os::network::Error::ConnectionAborted;
			case WSAENOTCONN:
			case WSAESHUTDOWN:
				return nox::os::network::Error::NotConnected;
			case WSAEADDRINUSE:
				return nox::os::network::Error::AddressInUse;
			case WSAEADDRNOTAVAIL:
				return nox::os::network::Error::AddressNotAvailable;
			case WSAENETUNREACH:
			case WSAENETDOWN:
				return nox::os::network::Error::NetworkUnreachable;
			case WSAEHOSTUNREACH:
				return nox::os::network::Error::HostUnreachable;
			case WSAEMSGSIZE:
				return nox::os::network::Error::MessageTooLong;
			case WSAEINVAL:
			case WSAEFAULT:
			case WSAENOTSOCK:
			case WSAEAFNOSUPPORT:
			case WSAEPROTONOSUPPORT:
			case WSAEOPNOTSUPP:
			case WSAEISCONN:
				return nox::os::network::Error::InvalidArgument;
			case WSAENOBUFS:
			case WSAEMFILE:
				return nox::os::network::Error::NoResources;
			case WSAEACCES:
				return nox::os::network::Error::AccessDenied;
			case WSANOTINITIALISED:
				return nox::os::network::Error::NotInitialized;
			default:
				return nox::os::network::Error::Unknown;
			}
		}

		inline nox::os::network::Error LastError()noexcept
		{
			return nox::os::network::ToError(::WSAGetLastError());
		}

		inline constexpr int ToNativeFlags(const nox::os::network::SendFlag flag)noexcept
		{
			int result = 0;
			if (nox::util::IsBitAnd(flag, nox::os::network::SendFlag::Oob))
			{
				result |= MSG_OOB;
			}
			if (nox::util::IsBitAnd(flag, nox::os::network::SendFlag::DontRoute))
			{
				result |= MSG_DONTROUTE;
			}
			return result;
		}

		inline constexpr int ToNativeFlags(const nox::os::network::ReceiveFlag flag)noexcept
		{
			int result = 0;
			if (nox::util::IsBitAnd(flag, nox::os::network::ReceiveFlag::Peek))
			{
				result |= MSG_PEEK;
			}
			if (nox::util::IsBitAnd(flag, nox::os::network::ReceiveFlag::Oob))
			{
				result |= MSG_OOB;
			}
			if (nox::util::IsBitAnd(flag, nox::os::network::ReceiveFlag::WaitAll))
			{
				result |= MSG_WAITALL;
			}
			return result;
		}

		struct NativeAddress final
		{
			::sockaddr_storage storage;
			int length;
		};

		inline std::optional<nox::os::network::NativeAddress> ToNative(const nox::os::network::EndPoint& end_point)noexcept
		{
			nox::os::network::NativeAddress result{};
			switch (end_point.family)
			{
			case nox::os::network::IpFamily::IPv4:
			{
				auto& addr = reinterpret_cast<::sockaddr_in&>(result.storage);
				addr.sin_family = AF_INET;
				addr.sin_port = ::htons(end_point.port);
				std::memcpy(&addr.sin_addr, end_point.address.data(), 4);
				result.length = static_cast<int>(sizeof(::sockaddr_in));
			}
			break;
			case nox::os::network::IpFamily::IPv6:
			{
				auto& addr = reinterpret_cast<::sockaddr_in6&>(result.storage);
				addr.sin6_family = AF_INET6;
				addr.sin6_port = ::htons(end_point.port);
				addr.sin6_scope_id = end_point.scope_id;
				std::memcpy(&addr.sin6_addr, end_point.address.data(), 16);
				result.length = static_cast<int>(sizeof(::sockaddr_in6));
			}
			break;
			default:
				return std::nullopt;
			}
			return result;
		}

		inline std::optional<nox::os::network::EndPoint> FromNative(const ::sockaddr_storage& storage, const int length)noexcept
		{
			nox::os::network::EndPoint result{};
			switch (storage.ss_family)
			{
			case AF_INET:
			{
				if (length < static_cast<int>(sizeof(::sockaddr_in)))
				{
					return std::nullopt;
				}
				const auto& addr = reinterpret_cast<const ::sockaddr_in&>(storage);
				result.family = nox::os::network::IpFamily::IPv4;
				result.port = ::ntohs(addr.sin_port);
				std::memcpy(result.address.data(), &addr.sin_addr, 4);
			}
			break;
			case AF_INET6:
			{
				if (length < static_cast<int>(sizeof(::sockaddr_in6)))
				{
					return std::nullopt;
				}
				const auto& addr = reinterpret_cast<const ::sockaddr_in6&>(storage);
				result.family = nox::os::network::IpFamily::IPv6;
				result.port = ::ntohs(addr.sin6_port);
				result.scope_id = addr.sin6_scope_id;
				std::memcpy(result.address.data(), &addr.sin6_addr, 16);
			}
			break;
			default:
				return std::nullopt;
			}
			return result;
		}

		inline nox::os::network::Error SetSocketOption(nox::os::network::Socket socket, int level, int option_name, const int value)noexcept
		{
			const int result = ::setsockopt(ToNative(socket), level, option_name, reinterpret_cast<const char*>(&value), static_cast<int>(sizeof(value)));
			if (result == SOCKET_ERROR)
			{
				return nox::os::network::LastError();
			}
			return nox::os::network::Error::None;
		}

		///	@brief	子プロセスへ継承させない
		///	@details	accept で受けたソケットは作るときに指定できないので、受けた直後に外す。
		///				その間に別スレッドが子プロセスを作ると継承されうるが、接続時だけの短い隙間として許容する。
		inline void DisableInherit(const ::SOCKET socket)noexcept
		{
			::SetHandleInformation(reinterpret_cast<::HANDLE>(socket), HANDLE_FLAG_INHERIT, 0);
		}

		inline nox::os::network::Error SetNonBlockingNative(const ::SOCKET socket, const bool enable)noexcept
		{
			::u_long mode = enable ? 1 : 0;
			if (::ioctlsocket(socket, FIONBIO, &mode) == SOCKET_ERROR)
			{
				return nox::os::network::LastError();
			}
			return nox::os::network::Error::None;
		}

		///	@details	UDP で受信バッファより大きいデータグラムを受けると、WinSock は切り詰めたデータを書いたうえで
		///				WSAEMSGSIZE で失敗を返す。書いたバイト数と MessageTooLong を返す。
		inline nox::os::network::IoResult ToReceiveResult(const int result, const std::size_t buffer_size)noexcept
		{
			if (result != SOCKET_ERROR)
			{
				return nox::os::network::IoResult{ result, nox::os::network::Error::None };
			}

			const nox::os::network::Error error = nox::os::network::LastError();
			if (error == nox::os::network::Error::MessageTooLong)
			{
				return nox::os::network::IoResult{ static_cast<nox::int32>(buffer_size), error };
			}
			return nox::os::network::IoResult{ 0, error };
		}

		inline nox::os::network::IoResult ToSendResult(const int result)noexcept
		{
			if (result == SOCKET_ERROR)
			{
				return nox::os::network::IoResult{ 0, nox::os::network::LastError() };
			}
			return nox::os::network::IoResult{ result, nox::os::network::Error::None };
		}

		inline nox::os::network::PollEvent ToPollEvent(const ::SHORT revents)noexcept
		{
			nox::os::network::PollEvent result = nox::os::network::PollEvent::None;
			if ((revents & POLLIN) != 0)
			{
				result = nox::util::BitOr(result, nox::os::network::PollEvent::Readable);
			}
			if ((revents & POLLOUT) != 0)
			{
				result = nox::util::BitOr(result, nox::os::network::PollEvent::Writable);
			}
			if ((revents & POLLERR) != 0)
			{
				result = nox::util::BitOr(result, nox::os::network::PollEvent::Error);
			}
			if ((revents & POLLHUP) != 0)
			{
				result = nox::util::BitOr(result, nox::os::network::PollEvent::HangUp);
			}
			if ((revents & POLLNVAL) != 0)
			{
				result = nox::util::BitOr(result, nox::os::network::PollEvent::Invalid);
			}
			return result;
		}

		//	WSAAPI (FAR PASCAL) は win64_api.h が far を外しているので書けない。宣言から型を取る
		using GetNameFunction = decltype(&::getsockname);

		inline std::expected<nox::os::network::EndPoint, nox::os::network::Error> GetEndPoint(const nox::os::network::Socket socket, const GetNameFunction function)noexcept
		{
			::sockaddr_storage storage{};
			int length = static_cast<int>(sizeof(storage));
			if (function(ToNative(socket), reinterpret_cast<::sockaddr*>(&storage), &length) == SOCKET_ERROR)
			{
				return std::unexpected(nox::os::network::LastError());
			}

			const auto end_point = nox::os::network::FromNative(storage, length);
			if (end_point.has_value() == false)
			{
				return std::unexpected(nox::os::network::Error::Unknown);
			}
			return *end_point;
		}
	}
}

nox::os::network::Error nox::os::network::Initialize()noexcept
{
	::WSADATA wsa_data{};
	//	WSAStartup は WSAGetLastError ではなく戻り値でエラーを返す
	return nox::os::network::ToError(::WSAStartup(MAKEWORD(2, 2), &wsa_data));
}

void nox::os::network::Finalize()noexcept
{
	::WSACleanup();
}

std::expected<nox::os::network::Socket, nox::os::network::Error> nox::os::network::CreateSocket(nox::os::network::IpFamily family, nox::os::network::Protocol protocol)noexcept
{
	const int native_family = nox::os::network::GetNativeFamily(family);
	if (native_family == 0)
	{
		return std::unexpected(nox::os::network::Error::InvalidArgument);
	}

	bool is_tcp ;
	switch (protocol)
	{
	case nox::os::network::Protocol::TCP:
		is_tcp = true;
		break;
	case nox::os::network::Protocol::UDP:
		is_tcp = false;
		break;
	default:
		return std::unexpected(nox::os::network::Error::InvalidArgument);
	}

	//const bool is_tcp = (protocol == nox::os::network::Protocol::TCP);
	const ::SOCKET socket = ::WSASocketW(
		native_family,
		is_tcp ? SOCK_STREAM : SOCK_DGRAM,
		is_tcp ? ::IPPROTO_TCP : ::IPPROTO_UDP,
		nullptr,
		0,
		//	socket() と同じく overlapped にし、作った瞬間から子プロセスへ継承させない
		WSA_FLAG_OVERLAPPED | WSA_FLAG_NO_HANDLE_INHERIT
	);

	if (socket == INVALID_SOCKET)
	{
		return std::unexpected(nox::os::network::LastError());
	}

	return nox::os::network::FromNative(socket);
}

std::expected<nox::os::network::AcceptedSocket, nox::os::network::Error> nox::os::network::Accept(const nox::os::network::Socket listener, const nox::os::network::SocketMode mode)noexcept
{
	::sockaddr_storage storage{};
	int length = static_cast<int>(sizeof(storage));
	const ::SOCKET socket = ::accept(nox::os::network::ToNative(listener), reinterpret_cast<::sockaddr*>(&storage), &length);
	if (socket == INVALID_SOCKET)
	{
		return std::unexpected(nox::os::network::LastError());
	}

	nox::os::network::DisableInherit(socket);

	//	待ち受け側の設定を引き継ぐかに頼らず、指定どおりに設定し直す
	const nox::os::network::Error mode_error = nox::os::network::SetNonBlockingNative(socket, mode == nox::os::network::SocketMode::NonBlocking);
	if (mode_error != nox::os::network::Error::None)
	{
		::closesocket(socket);
		return std::unexpected(mode_error);
	}

	nox::os::network::AcceptedSocket result;
	result.socket = nox::os::network::FromNative(socket);
	result.remote = nox::os::network::FromNative(storage, length).value_or(nox::os::network::EndPoint{});
	return result;
}

std::expected<nox::os::network::EndPoint, nox::os::network::Error> nox::os::network::GetLocalEndPoint(const nox::os::network::Socket socket)noexcept
{
	return nox::os::network::GetEndPoint(socket, &::getsockname);
}

std::expected<nox::os::network::EndPoint, nox::os::network::Error> nox::os::network::GetRemoteEndPoint(const nox::os::network::Socket socket)noexcept
{
	return nox::os::network::GetEndPoint(socket, &::getpeername);
}

std::expected<nox::int32, nox::os::network::Error> nox::os::network::FormatAddress(const nox::os::network::EndPoint& end_point, const std::span<char> buffer)noexcept
{
	int native_family = 0;
	switch (end_point.family)
	{
	case nox::os::network::IpFamily::IPv4:
		native_family = AF_INET;
		break;
	case nox::os::network::IpFamily::IPv6:
		native_family = AF_INET6;
		break;
	default:
		return std::unexpected(nox::os::network::Error::InvalidArgument);
	}

	if (buffer.empty() == true || nox::os::network::detail::FitsInt(buffer.size()) == false)
	{
		return std::unexpected(nox::os::network::Error::InvalidArgument);
	}

	//	inet_ntop の address は in_addr / in6_addr を指す。EndPoint::address の先頭がそれと同じ並び (ネットワークバイト順)
	if (::inet_ntop(native_family, end_point.address.data(), buffer.data(), buffer.size()) == nullptr)
	{
		//	バッファが足りないときは ERROR_INVALID_PARAMETER になる
		return std::unexpected(nox::os::network::Error::InvalidArgument);
	}
	return static_cast<nox::int32>(std::strlen(buffer.data()));
}

std::expected<nox::os::network::EndPoint, nox::os::network::Error> nox::os::network::ParseAddress(const std::string_view text, const nox::os::network::Port port)noexcept
{
	//	inet_pton は終端の 0 が要るので、スタック上へ写す
	char buffer[64];
	if (text.empty() == true || text.size() >= sizeof(buffer) || text.find('\0') != std::string_view::npos)
	{
		return std::unexpected(nox::os::network::Error::InvalidArgument);
	}
	std::memcpy(buffer, text.data(), text.size());
	buffer[text.size()] = '\0';

	nox::os::network::EndPoint result{};
	result.port = port;
	if (::inet_pton(AF_INET, buffer, result.address.data()) == 1)
	{
		result.family = nox::os::network::IpFamily::IPv4;
		return result;
	}
	if (::inet_pton(AF_INET6, buffer, result.address.data()) == 1)
	{
		result.family = nox::os::network::IpFamily::IPv6;
		return result;
	}
	return std::unexpected(nox::os::network::Error::InvalidArgument);
}

std::expected<nox::int32, nox::os::network::Error> nox::os::network::Poll(const std::span<nox::os::network::PollEntry> entries, const nox::int32 timeout_milliseconds)noexcept
{
	if (entries.empty() == true || entries.size() > static_cast<std::size_t>(nox::os::network::kMaxPollEntries))
	{
		return std::unexpected(nox::os::network::Error::InvalidArgument);
	}

	//	無効なソケットは WSAPoll に渡さず、その場で Invalid を返す
	::WSAPOLLFD native_entries[nox::os::network::kMaxPollEntries];
	nox::uint8 native_to_entry[nox::os::network::kMaxPollEntries];
	::ULONG native_count = 0;
	nox::int32 invalid_count = 0;
	for (std::size_t i = 0; i < entries.size(); ++i)
	{
		nox::os::network::PollEntry& entry = entries[i];
		entry.returned = nox::os::network::PollEvent::None;
		if (entry.socket.IsValid() == false)
		{
			entry.returned = nox::os::network::PollEvent::Invalid;
			++invalid_count;
			continue;
		}

		::WSAPOLLFD& native = native_entries[native_count];
		native.fd = nox::os::network::ToNative(entry.socket);
		native.events = 0;
		native.revents = 0;
		if (nox::util::IsBitAnd(entry.requested, nox::os::network::PollEvent::Readable))
		{
			native.events = static_cast<::SHORT>(native.events | POLLIN);
		}
		if (nox::util::IsBitAnd(entry.requested, nox::os::network::PollEvent::Writable))
		{
			native.events = static_cast<::SHORT>(native.events | POLLOUT);
		}
		native_to_entry[native_count] = static_cast<nox::uint8>(i);
		++native_count;
	}

	if (native_count == 0)
	{
		return invalid_count;
	}

	//	無効なソケットがあるときは待たずに返す
	const int result = ::WSAPoll(native_entries, native_count, (invalid_count > 0) ? 0 : timeout_milliseconds);
	if (result == SOCKET_ERROR)
	{
		return std::unexpected(nox::os::network::LastError());
	}

	nox::int32 ready_count = invalid_count;
	for (::ULONG i = 0; i < native_count; ++i)
	{
		const nox::os::network::PollEvent returned = nox::os::network::ToPollEvent(native_entries[i].revents);
		entries[native_to_entry[i]].returned = returned;
		if (returned != nox::os::network::PollEvent::None)
		{
			++ready_count;
		}
	}
	return ready_count;
}

nox::os::network::Error nox::os::network::Close(nox::os::network::Socket& socket)noexcept
{
	if (socket.IsValid() == false)
	{
		return nox::os::network::Error::None;
	}

	const int result = ::closesocket(nox::os::network::ToNative(socket));
	socket.value = nox::os::network::Socket::kInvalidSocket;
	return (result == SOCKET_ERROR) ? nox::os::network::LastError() : nox::os::network::Error::None;
}

nox::os::network::Error nox::os::network::Bind(const nox::os::network::Socket socket, const nox::os::network::EndPoint& end_point)noexcept
{
	const auto native_address_opt = nox::os::network::ToNative(end_point);
	if (native_address_opt.has_value() == false)
	{
		return nox::os::network::Error::InvalidArgument;
	}

	if (::bind(
		nox::os::network::ToNative(socket),
		reinterpret_cast<const ::sockaddr*>(&native_address_opt->storage),
		native_address_opt->length) == SOCKET_ERROR)
	{
		return nox::os::network::LastError();
	}
	return nox::os::network::Error::None;
}

nox::os::network::Error nox::os::network::Listen(const nox::os::network::Socket socket, const nox::int32 backlog)noexcept
{
	if (::listen(nox::os::network::ToNative(socket), (backlog < 0) ? SOMAXCONN : backlog) == SOCKET_ERROR)
	{
		return nox::os::network::LastError();
	}
	return nox::os::network::Error::None;
}

nox::os::network::Error nox::os::network::Connect(const nox::os::network::Socket socket, const nox::os::network::EndPoint& end_point)noexcept
{
	const auto native_address_opt = nox::os::network::ToNative(end_point);
	if (native_address_opt.has_value() == false)
	{
		return nox::os::network::Error::InvalidArgument;
	}

	if (::connect(
		nox::os::network::ToNative(socket),
		reinterpret_cast<const ::sockaddr*>(&native_address_opt->storage),
		native_address_opt->length) == SOCKET_ERROR)
	{
		const int code = ::WSAGetLastError();
		//	ノンブロッキングの connect は、WinSock では WSAEWOULDBLOCK になる (POSIX の EINPROGRESS に当たる)
		if (code == WSAEWOULDBLOCK)
		{
			return nox::os::network::Error::InProgress;
		}
		//	進行中の接続が済んだ後にもう一度呼ぶと WSAEISCONN になる。接続できているので成功として返す
		if (code == WSAEISCONN)
		{
			return nox::os::network::Error::None;
		}
		return nox::os::network::ToError(code);
	}
	return nox::os::network::Error::None;
}

nox::os::network::Error nox::os::network::GetPendingError(const nox::os::network::Socket socket)noexcept
{
	int value = 0;
	int length = static_cast<int>(sizeof(value));
	if (::getsockopt(nox::os::network::ToNative(socket), SOL_SOCKET, SO_ERROR, reinterpret_cast<char*>(&value), &length) == SOCKET_ERROR)
	{
		return nox::os::network::LastError();
	}
	return nox::os::network::ToError(value);
}

nox::os::network::Error nox::os::network::Shutdown(const nox::os::network::Socket socket, const nox::os::network::ShutdownMode mode)noexcept
{
	int how = SD_BOTH;
	switch (mode)
	{
	case nox::os::network::ShutdownMode::Receive:
		how = SD_RECEIVE;
		break;
	case nox::os::network::ShutdownMode::Send:
		how = SD_SEND;
		break;
	case nox::os::network::ShutdownMode::Both:
		how = SD_BOTH;
		break;
	}

	if (::shutdown(nox::os::network::ToNative(socket), how) == SOCKET_ERROR)
	{
		return nox::os::network::LastError();
	}
	return nox::os::network::Error::None;
}

nox::os::network::Error nox::os::network::SetNonBlocking(const nox::os::network::Socket socket, const bool enable)noexcept
{
	return nox::os::network::SetNonBlockingNative(nox::os::network::ToNative(socket), enable);
}

nox::os::network::Error nox::os::network::SetNoDelay(const nox::os::network::Socket socket, const bool enable)noexcept
{
	return nox::os::network::SetSocketOption(socket, IPPROTO_TCP, TCP_NODELAY, enable ? 1 : 0);
}

nox::os::network::Error nox::os::network::SetKeepAlive(const nox::os::network::Socket socket, const bool enable)noexcept
{
	return nox::os::network::SetSocketOption(socket, SOL_SOCKET, SO_KEEPALIVE, enable ? 1 : 0);
}

nox::os::network::Error nox::os::network::SetReuseAddress([[maybe_unused]] const nox::os::network::Socket socket, [[maybe_unused]] const bool enable)noexcept
{
	//	Windows では、閉じた直後のポートへの bind は既定で通る。SO_REUSEADDR は使用中のポートの横取りまで許すので設定しない
	return nox::os::network::Error::None;
}

nox::os::network::Error nox::os::network::SetIPv6Only(const nox::os::network::Socket socket, const bool enable)noexcept
{
	return nox::os::network::SetSocketOption(socket, IPPROTO_IPV6, IPV6_V6ONLY, enable ? 1 : 0);
}

nox::os::network::Error nox::os::network::SetSendBufferSize(const nox::os::network::Socket socket, const nox::int32 size)noexcept
{
	return nox::os::network::SetSocketOption(socket, SOL_SOCKET, SO_SNDBUF, size);
}

nox::os::network::Error nox::os::network::SetReceiveBufferSize(const nox::os::network::Socket socket, const nox::int32 size)noexcept
{
	return nox::os::network::SetSocketOption(socket, SOL_SOCKET, SO_RCVBUF, size);
}

nox::os::network::IoResult nox::os::network::Send(const nox::os::network::Socket socket, const std::span<const nox::uint8> buffer, const nox::os::network::SendFlag flags)noexcept
{
	if (nox::os::network::detail::FitsInt(buffer.size()) == false)
	{
		return nox::os::network::IoResult{ 0, nox::os::network::Error::InvalidArgument };
	}

	return nox::os::network::ToSendResult(::send(
		nox::os::network::ToNative(socket),
		reinterpret_cast<const char*>(buffer.data()),
		static_cast<int>(buffer.size()),
		nox::os::network::ToNativeFlags(flags)));
}

nox::os::network::IoResult nox::os::network::Receive(const nox::os::network::Socket socket, const std::span<nox::uint8> buffer, const nox::os::network::ReceiveFlag flags)noexcept
{
	if (nox::os::network::detail::FitsInt(buffer.size()) == false)
	{
		return nox::os::network::IoResult{ 0, nox::os::network::Error::InvalidArgument };
	}

	return nox::os::network::ToReceiveResult(::recv(
		nox::os::network::ToNative(socket),
		reinterpret_cast<char*>(buffer.data()),
		static_cast<int>(buffer.size()),
		nox::os::network::ToNativeFlags(flags)), buffer.size());
}

nox::os::network::IoResult nox::os::network::SendTo(const nox::os::network::Socket socket, const std::span<const nox::uint8> buffer, const nox::os::network::EndPoint& destination, const nox::os::network::SendFlag flags)noexcept
{
	const auto native_address_opt = nox::os::network::ToNative(destination);
	if (nox::os::network::detail::FitsInt(buffer.size()) == false || native_address_opt.has_value() == false)
	{
		return nox::os::network::IoResult{ 0, nox::os::network::Error::InvalidArgument };
	}

	return nox::os::network::ToSendResult(::sendto(
		nox::os::network::ToNative(socket),
		reinterpret_cast<const char*>(buffer.data()),
		static_cast<int>(buffer.size()),
		nox::os::network::ToNativeFlags(flags),
		reinterpret_cast<const ::sockaddr*>(&native_address_opt->storage),
		native_address_opt->length));
}

nox::os::network::ReceiveFromResult nox::os::network::ReceiveFrom(const nox::os::network::Socket socket, const std::span<nox::uint8> buffer, const nox::os::network::ReceiveFlag flags)noexcept
{
	if (nox::os::network::detail::FitsInt(buffer.size()) == false)
	{
		return nox::os::network::ReceiveFromResult{ 0, nox::os::network::Error::InvalidArgument, nox::os::network::EndPoint{} };
	}

	::sockaddr_storage storage{};
	int length = static_cast<int>(sizeof(storage));
	const nox::os::network::IoResult io_result = nox::os::network::ToReceiveResult(::recvfrom(
		nox::os::network::ToNative(socket),
		reinterpret_cast<char*>(buffer.data()),
		static_cast<int>(buffer.size()),
		nox::os::network::ToNativeFlags(flags),
		reinterpret_cast<::sockaddr*>(&storage),
		&length), buffer.size());

	nox::os::network::ReceiveFromResult result;
	result.bytes = io_result.bytes;
	result.error = io_result.error;
	result.source = nox::os::network::FromNative(storage, length).value_or(nox::os::network::EndPoint{});
	return result;
}
#endif // NOX_WINDOWS
