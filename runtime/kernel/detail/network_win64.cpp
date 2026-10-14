//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	network_win64.cpp
///	@brief	network の Windows 実装 (WinSock)
#include	"pch.h"
#include	"../network.h"

#include	<cstring>
#include	<limits>
#include	<type_traits>
#include	"../win64_socket.h"

#if NOX_WIN64
namespace nox::os::network
{
	namespace
	{
		static_assert(static_cast<nox::intptr>(INVALID_SOCKET) == Socket::kInvalidValue, "INVALID_SOCKET を intptr にすると -1 になる前提");

		inline ::SOCKET ToNative(const Socket socket) noexcept
		{
			return static_cast<::SOCKET>(socket.value);
		}

		inline Socket FromNative(const ::SOCKET socket) noexcept
		{
			return Socket{ static_cast<nox::intptr>(socket) };
		}

		Error ToError(const int code) noexcept
		{
			switch (code)
			{
			case 0:
				return Error::None;
			case WSAEWOULDBLOCK:
				return Error::WouldBlock;
			case WSAEINPROGRESS:
			case WSAEALREADY:
				return Error::InProgress;
			case WSAEINTR:
				return Error::Interrupted;
			case WSAETIMEDOUT:
				return Error::TimedOut;
			case WSAECONNRESET:
			case WSAENETRESET:
				return Error::ConnectionReset;
			case WSAECONNREFUSED:
				return Error::ConnectionRefused;
			case WSAECONNABORTED:
				return Error::ConnectionAborted;
			case WSAENOTCONN:
			case WSAESHUTDOWN:
				return Error::NotConnected;
			case WSAEADDRINUSE:
				return Error::AddressInUse;
			case WSAEADDRNOTAVAIL:
				return Error::AddressNotAvailable;
			case WSAENETUNREACH:
			case WSAENETDOWN:
				return Error::NetworkUnreachable;
			case WSAEHOSTUNREACH:
				return Error::HostUnreachable;
			case WSAEMSGSIZE:
				return Error::MessageTooLong;
			case WSAEINVAL:
			case WSAEFAULT:
			case WSAENOTSOCK:
			case WSAEAFNOSUPPORT:
			case WSAEPROTONOSUPPORT:
			case WSAEOPNOTSUPP:
			case WSAEISCONN:
				return Error::InvalidArgument;
			case WSAENOBUFS:
			case WSAEMFILE:
				return Error::NoResources;
			case WSAEACCES:
				return Error::AccessDenied;
			case WSANOTINITIALISED:
				return Error::NotInitialized;
			default:
				return Error::Unknown;
			}
		}

		inline Error LastError() noexcept
		{
			return ToError(::WSAGetLastError());
		}

		template<class T>
		inline constexpr bool HasFlag(const T value, const T flag) noexcept
		{
			return (static_cast<std::underlying_type_t<T>>(value) & static_cast<std::underlying_type_t<T>>(flag)) != 0;
		}

		int ToNativeFlags(const SendFlag flags) noexcept
		{
			int result = 0;
			if (HasFlag(flags, SendFlag::Oob))
			{
				result |= MSG_OOB;
			}
			if (HasFlag(flags, SendFlag::DontRoute))
			{
				result |= MSG_DONTROUTE;
			}
			return result;
		}

		int ToNativeFlags(const ReceiveFlag flags) noexcept
		{
			int result = 0;
			if (HasFlag(flags, ReceiveFlag::Peek))
			{
				result |= MSG_PEEK;
			}
			if (HasFlag(flags, ReceiveFlag::Oob))
			{
				result |= MSG_OOB;
			}
			if (HasFlag(flags, ReceiveFlag::WaitAll))
			{
				result |= MSG_WAITALL;
			}
			return result;
		}

		struct NativeAddress
		{
			::sockaddr_storage storage;
			int length;
		};

		bool ToNative(const EndPoint& end_point, NativeAddress& out_address) noexcept
		{
			std::memset(&out_address.storage, 0, sizeof(out_address.storage));
			switch (end_point.family)
			{
			case IpFamily::IPv4:
			{
				auto& native = reinterpret_cast<::sockaddr_in&>(out_address.storage);
				native.sin_family = AF_INET;
				native.sin_port = ::htons(end_point.port);
				std::memcpy(&native.sin_addr, end_point.address, 4);
				out_address.length = static_cast<int>(sizeof(::sockaddr_in));
				return true;
			}
			case IpFamily::IPv6:
			{
				auto& native = reinterpret_cast<::sockaddr_in6&>(out_address.storage);
				native.sin6_family = AF_INET6;
				native.sin6_port = ::htons(end_point.port);
				std::memcpy(&native.sin6_addr, end_point.address, 16);
				native.sin6_scope_id = end_point.scope_id;
				out_address.length = static_cast<int>(sizeof(::sockaddr_in6));
				return true;
			}
			default:
				return false;
			}
		}

		bool FromNative(const ::sockaddr_storage& storage, EndPoint& out_end_point) noexcept
		{
			out_end_point = EndPoint{};
			switch (storage.ss_family)
			{
			case AF_INET:
			{
				const auto& native = reinterpret_cast<const ::sockaddr_in&>(storage);
				out_end_point.family = IpFamily::IPv4;
				out_end_point.port = ::ntohs(native.sin_port);
				std::memcpy(out_end_point.address, &native.sin_addr, 4);
				return true;
			}
			case AF_INET6:
			{
				const auto& native = reinterpret_cast<const ::sockaddr_in6&>(storage);
				out_end_point.family = IpFamily::IPv6;
				out_end_point.port = ::ntohs(native.sin6_port);
				out_end_point.scope_id = native.sin6_scope_id;
				std::memcpy(out_end_point.address, &native.sin6_addr, 16);
				return true;
			}
			default:
				return false;
			}
		}

		inline bool FitsInt(const std::size_t size) noexcept
		{
			return size <= static_cast<std::size_t>((std::numeric_limits<int>::max)());
		}

		Error SetIntOption(const Socket socket, const int level, const int name, const int value) noexcept
		{
			if (::setsockopt(ToNative(socket), level, name, reinterpret_cast<const char*>(&value), static_cast<int>(sizeof(value))) == SOCKET_ERROR)
			{
				return LastError();
			}
			return Error::None;
		}

		//	ソケットはハンドルなので、子プロセスへ継承させない
		inline void DisableInherit(const ::SOCKET socket) noexcept
		{
			::SetHandleInformation(reinterpret_cast<::HANDLE>(socket), HANDLE_FLAG_INHERIT, 0);
		}

		//	UDP で受信バッファより大きいデータグラムを受けると、WinSock は切り詰めたデータを書いたうえで
		//	WSAEMSGSIZE で失敗を返す。POSIX 版と揃え、書いたバイト数と MessageTooLong を返す。
		inline IoResult ToReceiveResult(const int result, const std::size_t buffer_size) noexcept
		{
			if (result != SOCKET_ERROR)
			{
				return IoResult{ result, Error::None };
			}

			const Error error = LastError();
			if (error == Error::MessageTooLong)
			{
				return IoResult{ static_cast<nox::int32>(buffer_size), error };
			}
			return IoResult{ 0, error };
		}
	}

	Error Initialize() noexcept
	{
		::WSADATA wsa_data;
		//	WSAStartup は WSAGetLastError ではなく戻り値でエラーを返す
		return ToError(::WSAStartup(MAKEWORD(2, 2), &wsa_data));
	}

	void Finalize() noexcept
	{
		::WSACleanup();
	}

	Error CreateSocket(const IpFamily family, const Protocol protocol, Socket& out_socket) noexcept
	{
		out_socket = Socket{};

		int native_family = 0;
		switch (family)
		{
		case IpFamily::IPv4:
			native_family = AF_INET;
			break;
		case IpFamily::IPv6:
			native_family = AF_INET6;
			break;
		default:
			return Error::InvalidArgument;
		}

		const bool is_tcp = (protocol == Protocol::Tcp);
		const ::SOCKET socket = ::WSASocketW(
			native_family,
			is_tcp ? SOCK_STREAM : SOCK_DGRAM,
			is_tcp ? IPPROTO_TCP : IPPROTO_UDP,
			nullptr,
			0,
			WSA_FLAG_OVERLAPPED | WSA_FLAG_NO_HANDLE_INHERIT);
		if (socket == INVALID_SOCKET)
		{
			return LastError();
		}

		out_socket = FromNative(socket);
		return Error::None;
	}

	Error Close(Socket& socket) noexcept
	{
		if (socket.IsValid() == false)
		{
			return Error::None;
		}

		const int result = ::closesocket(ToNative(socket));
		socket = Socket{};
		return (result == SOCKET_ERROR) ? LastError() : Error::None;
	}

	Error Bind(const Socket socket, const EndPoint& end_point) noexcept
	{
		NativeAddress address;
		if (ToNative(end_point, address) == false)
		{
			return Error::InvalidArgument;
		}

		if (::bind(ToNative(socket), reinterpret_cast<const ::sockaddr*>(&address.storage), address.length) == SOCKET_ERROR)
		{
			return LastError();
		}
		return Error::None;
	}

	Error Listen(const Socket socket, const nox::int32 backlog) noexcept
	{
		if (::listen(ToNative(socket), (backlog < 0) ? SOMAXCONN : backlog) == SOCKET_ERROR)
		{
			return LastError();
		}
		return Error::None;
	}

	Error Accept(const Socket listener, Socket& out_socket, EndPoint* const out_remote) noexcept
	{
		out_socket = Socket{};

		::sockaddr_storage storage;
		int length = static_cast<int>(sizeof(storage));
		const ::SOCKET socket = ::accept(ToNative(listener), reinterpret_cast<::sockaddr*>(&storage), &length);
		if (socket == INVALID_SOCKET)
		{
			return LastError();
		}

		DisableInherit(socket);
		out_socket = FromNative(socket);
		if (out_remote != nullptr)
		{
			FromNative(storage, *out_remote);
		}
		return Error::None;
	}

	Error Connect(const Socket socket, const EndPoint& end_point) noexcept
	{
		NativeAddress address;
		if (ToNative(end_point, address) == false)
		{
			return Error::InvalidArgument;
		}

		if (::connect(ToNative(socket), reinterpret_cast<const ::sockaddr*>(&address.storage), address.length) == SOCKET_ERROR)
		{
			//	ノンブロッキングの connect は、WinSock では WSAEWOULDBLOCK、POSIX では EINPROGRESS になる
			const Error error = LastError();
			return (error == Error::WouldBlock) ? Error::InProgress : error;
		}
		return Error::None;
	}

	Error GetPendingError(const Socket socket, Error& out_error) noexcept
	{
		int value = 0;
		int length = static_cast<int>(sizeof(value));
		if (::getsockopt(ToNative(socket), SOL_SOCKET, SO_ERROR, reinterpret_cast<char*>(&value), &length) == SOCKET_ERROR)
		{
			out_error = Error::None;
			return LastError();
		}

		out_error = ToError(value);
		return Error::None;
	}

	Error Shutdown(const Socket socket, const ShutdownMode mode) noexcept
	{
		int how = SD_BOTH;
		switch (mode)
		{
		case ShutdownMode::Receive:
			how = SD_RECEIVE;
			break;
		case ShutdownMode::Send:
			how = SD_SEND;
			break;
		case ShutdownMode::Both:
			how = SD_BOTH;
			break;
		}

		if (::shutdown(ToNative(socket), how) == SOCKET_ERROR)
		{
			return LastError();
		}
		return Error::None;
	}

	IoResult Send(const Socket socket, const std::span<const nox::uint8> buffer, const SendFlag flags) noexcept
	{
		if (FitsInt(buffer.size()) == false)
		{
			return IoResult{ 0, Error::InvalidArgument };
		}

		const int result = ::send(ToNative(socket), reinterpret_cast<const char*>(buffer.data()), static_cast<int>(buffer.size()), ToNativeFlags(flags));
		if (result == SOCKET_ERROR)
		{
			return IoResult{ 0, LastError() };
		}
		return IoResult{ result, Error::None };
	}

	IoResult Receive(const Socket socket, const std::span<nox::uint8> buffer, const ReceiveFlag flags) noexcept
	{
		if (FitsInt(buffer.size()) == false)
		{
			return IoResult{ 0, Error::InvalidArgument };
		}

		const int result = ::recv(ToNative(socket), reinterpret_cast<char*>(buffer.data()), static_cast<int>(buffer.size()), ToNativeFlags(flags));
		return ToReceiveResult(result, buffer.size());
	}

	IoResult SendTo(const Socket socket, const std::span<const nox::uint8> buffer, const EndPoint& destination, const SendFlag flags) noexcept
	{
		NativeAddress address;
		if (FitsInt(buffer.size()) == false || ToNative(destination, address) == false)
		{
			return IoResult{ 0, Error::InvalidArgument };
		}

		const int result = ::sendto(
			ToNative(socket),
			reinterpret_cast<const char*>(buffer.data()),
			static_cast<int>(buffer.size()),
			ToNativeFlags(flags),
			reinterpret_cast<const ::sockaddr*>(&address.storage),
			address.length);
		if (result == SOCKET_ERROR)
		{
			return IoResult{ 0, LastError() };
		}
		return IoResult{ result, Error::None };
	}

	IoResult ReceiveFrom(const Socket socket, const std::span<nox::uint8> buffer, EndPoint* const out_source, const ReceiveFlag flags) noexcept
	{
		if (FitsInt(buffer.size()) == false)
		{
			return IoResult{ 0, Error::InvalidArgument };
		}

		::sockaddr_storage storage;
		std::memset(&storage, 0, sizeof(storage));
		int length = static_cast<int>(sizeof(storage));
		const int result = ::recvfrom(
			ToNative(socket),
			reinterpret_cast<char*>(buffer.data()),
			static_cast<int>(buffer.size()),
			ToNativeFlags(flags),
			reinterpret_cast<::sockaddr*>(&storage),
			&length);

		const IoResult io_result = ToReceiveResult(result, buffer.size());
		if (out_source != nullptr)
		{
			FromNative(storage, *out_source);
		}
		return io_result;
	}

	Error Poll(const std::span<PollEntry> entries, const nox::int32 timeout_milliseconds, nox::int32& out_ready_count) noexcept
	{
		out_ready_count = 0;
		if (entries.empty() == true || entries.size() > static_cast<std::size_t>(kMaxPollEntries))
		{
			return Error::InvalidArgument;
		}

		//	無効なソケットは WSAPoll に渡さず、その場で Invalid を返す
		::WSAPOLLFD native_entries[kMaxPollEntries];
		nox::uint8 native_to_entry[kMaxPollEntries];
		::ULONG native_count = 0;
		nox::int32 invalid_count = 0;
		for (std::size_t i = 0; i < entries.size(); ++i)
		{
			PollEntry& entry = entries[i];
			entry.returned = PollEvent::None;
			if (entry.socket.IsValid() == false)
			{
				entry.returned = PollEvent::Invalid;
				++invalid_count;
				continue;
			}

			::WSAPOLLFD& native = native_entries[native_count];
			native.fd = ToNative(entry.socket);
			native.events = 0;
			native.revents = 0;
			if (HasAny(entry.requested, PollEvent::Readable))
			{
				native.events = static_cast<::SHORT>(native.events | POLLIN);
			}
			if (HasAny(entry.requested, PollEvent::Writable))
			{
				native.events = static_cast<::SHORT>(native.events | POLLOUT);
			}
			native_to_entry[native_count] = static_cast<nox::uint8>(i);
			++native_count;
		}

		if (native_count == 0)
		{
			out_ready_count = invalid_count;
			return Error::None;
		}

		//	無効なソケットがあるときは待たずに返す (POSIX の poll が POLLNVAL ですぐ返るのと揃える)
		const int result = ::WSAPoll(native_entries, native_count, (invalid_count > 0) ? 0 : timeout_milliseconds);
		if (result == SOCKET_ERROR)
		{
			return LastError();
		}

		for (::ULONG i = 0; i < native_count; ++i)
		{
			const ::SHORT revents = native_entries[i].revents;
			PollEvent returned = PollEvent::None;
			if ((revents & POLLIN) != 0)
			{
				returned = returned | PollEvent::Readable;
			}
			if ((revents & POLLOUT) != 0)
			{
				returned = returned | PollEvent::Writable;
			}
			if ((revents & POLLERR) != 0)
			{
				returned = returned | PollEvent::Error;
			}
			if ((revents & POLLHUP) != 0)
			{
				returned = returned | PollEvent::HangUp;
			}
			if ((revents & POLLNVAL) != 0)
			{
				returned = returned | PollEvent::Invalid;
			}

			entries[native_to_entry[i]].returned = returned;
			if (returned != PollEvent::None)
			{
				++out_ready_count;
			}
		}
		out_ready_count += invalid_count;
		return Error::None;
	}

	Error SetNonBlocking(const Socket socket, const bool enable) noexcept
	{
		::u_long mode = enable ? 1 : 0;
		if (::ioctlsocket(ToNative(socket), FIONBIO, &mode) == SOCKET_ERROR)
		{
			return LastError();
		}
		return Error::None;
	}

	Error SetNoDelay(const Socket socket, const bool enable) noexcept
	{
		return SetIntOption(socket, IPPROTO_TCP, TCP_NODELAY, enable ? 1 : 0);
	}

	Error SetIPv6Only(const Socket socket, const bool enable) noexcept
	{
		return SetIntOption(socket, IPPROTO_IPV6, IPV6_V6ONLY, enable ? 1 : 0);
	}

	Error SetSendBufferSize(const Socket socket, const nox::int32 size) noexcept
	{
		return SetIntOption(socket, SOL_SOCKET, SO_SNDBUF, size);
	}

	Error SetReceiveBufferSize(const Socket socket, const nox::int32 size) noexcept
	{
		return SetIntOption(socket, SOL_SOCKET, SO_RCVBUF, size);
	}

	Error GetLocalEndPoint(const Socket socket, EndPoint& out_end_point) noexcept
	{
		::sockaddr_storage storage;
		int length = static_cast<int>(sizeof(storage));
		if (::getsockname(ToNative(socket), reinterpret_cast<::sockaddr*>(&storage), &length) == SOCKET_ERROR)
		{
			out_end_point = EndPoint{};
			return LastError();
		}
		return FromNative(storage, out_end_point) ? Error::None : Error::Unknown;
	}

	Error GetRemoteEndPoint(const Socket socket, EndPoint& out_end_point) noexcept
	{
		::sockaddr_storage storage;
		int length = static_cast<int>(sizeof(storage));
		if (::getpeername(ToNative(socket), reinterpret_cast<::sockaddr*>(&storage), &length) == SOCKET_ERROR)
		{
			out_end_point = EndPoint{};
			return LastError();
		}
		return FromNative(storage, out_end_point) ? Error::None : Error::Unknown;
	}

	Error ParseAddress(const std::string_view text, const port_t port, EndPoint& out_end_point) noexcept
	{
		out_end_point = EndPoint{};

		//	inet_pton は終端の 0 が要るので、スタック上へ写す
		char buffer[64];
		if (text.empty() == true || text.size() >= sizeof(buffer) || text.find('\0') != std::string_view::npos)
		{
			return Error::InvalidArgument;
		}
		std::memcpy(buffer, text.data(), text.size());
		buffer[text.size()] = '\0';

		if (::inet_pton(AF_INET, buffer, out_end_point.address) == 1)
		{
			out_end_point.family = IpFamily::IPv4;
			out_end_point.port = port;
			return Error::None;
		}
		if (::inet_pton(AF_INET6, buffer, out_end_point.address) == 1)
		{
			out_end_point.family = IpFamily::IPv6;
			out_end_point.port = port;
			return Error::None;
		}

		out_end_point = EndPoint{};
		return Error::InvalidArgument;
	}
}
#endif // NOX_WIN64
