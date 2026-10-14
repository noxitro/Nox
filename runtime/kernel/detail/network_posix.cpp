//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	network_posix.cpp
///	@brief	network の POSIX 実装 (Linux / Android / macOS)
///	@details	Linux / Android / macOS のビルドはまだ無いので、このファイルは未検証。
#include	"pch.h"
#include	"../network.h"

//	basic_definition.h にまだ POSIX 系をまとめた判定が無いので、ここで直接見る
#if defined(__unix__) || defined(__APPLE__)
#include	<cerrno>
#include	<cstring>
#include	<limits>
#include	<type_traits>

#include	<fcntl.h>
#include	<poll.h>
#include	<unistd.h>
#include	<arpa/inet.h>
#include	<netinet/in.h>
#include	<netinet/tcp.h>
#include	<sys/socket.h>
#include	<sys/types.h>

namespace nox::os::network
{
	namespace
	{
		inline int ToNative(const Socket socket) noexcept
		{
			return static_cast<int>(socket.value);
		}

		inline Socket FromNative(const int socket) noexcept
		{
			return Socket{ static_cast<nox::intptr>(socket) };
		}

		Error ToError(const int code) noexcept
		{
			//	EAGAIN と EWOULDBLOCK は同じ値の OS があり、switch に並べられない
			if (code == EAGAIN || code == EWOULDBLOCK)
			{
				return Error::WouldBlock;
			}

			switch (code)
			{
			case 0:
				return Error::None;
			case EINPROGRESS:
			case EALREADY:
				return Error::InProgress;
			case EINTR:
				return Error::Interrupted;
			case ETIMEDOUT:
				return Error::TimedOut;
			case ECONNRESET:
			case ENETRESET:
			case EPIPE:
				return Error::ConnectionReset;
			case ECONNREFUSED:
				return Error::ConnectionRefused;
			case ECONNABORTED:
				return Error::ConnectionAborted;
			case ENOTCONN:
				return Error::NotConnected;
			case EADDRINUSE:
				return Error::AddressInUse;
			case EADDRNOTAVAIL:
				return Error::AddressNotAvailable;
			case ENETUNREACH:
			case ENETDOWN:
				return Error::NetworkUnreachable;
			case EHOSTUNREACH:
				return Error::HostUnreachable;
			case EMSGSIZE:
				return Error::MessageTooLong;
			case EINVAL:
			case EFAULT:
			case EBADF:
			case ENOTSOCK:
			case EAFNOSUPPORT:
			case EPROTONOSUPPORT:
			case EOPNOTSUPP:
			case EISCONN:
				return Error::InvalidArgument;
			case ENOBUFS:
			case ENOMEM:
			case EMFILE:
			case ENFILE:
				return Error::NoResources;
			case EACCES:
			case EPERM:
				return Error::AccessDenied;
			default:
				return Error::Unknown;
			}
		}

		inline Error LastError() noexcept
		{
			return ToError(errno);
		}

		template<class T>
		inline constexpr bool HasFlag(const T value, const T flag) noexcept
		{
			return (static_cast<std::underlying_type_t<T>>(value) & static_cast<std::underlying_type_t<T>>(flag)) != 0;
		}

		int ToNativeFlags(const SendFlag flags) noexcept
		{
			int result = 0;
			//	切れた相手へ送ると SIGPIPE でプロセスごと落ちるので、送信では常に抑止する。
			//	MSG_NOSIGNAL の無い OS (macOS) は、CreateSocket で SO_NOSIGPIPE を設定して抑止する。
#if defined(MSG_NOSIGNAL)
			result |= MSG_NOSIGNAL;
#endif // defined(MSG_NOSIGNAL)
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
			::socklen_t length;
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
				native.sin_port = htons(end_point.port);
				std::memcpy(&native.sin_addr, end_point.address, 4);
				out_address.length = static_cast<::socklen_t>(sizeof(::sockaddr_in));
				return true;
			}
			case IpFamily::IPv6:
			{
				auto& native = reinterpret_cast<::sockaddr_in6&>(out_address.storage);
				native.sin6_family = AF_INET6;
				native.sin6_port = htons(end_point.port);
				std::memcpy(&native.sin6_addr, end_point.address, 16);
				native.sin6_scope_id = end_point.scope_id;
				out_address.length = static_cast<::socklen_t>(sizeof(::sockaddr_in6));
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
				out_end_point.port = ntohs(native.sin_port);
				std::memcpy(out_end_point.address, &native.sin_addr, 4);
				return true;
			}
			case AF_INET6:
			{
				const auto& native = reinterpret_cast<const ::sockaddr_in6&>(storage);
				out_end_point.family = IpFamily::IPv6;
				out_end_point.port = ntohs(native.sin6_port);
				out_end_point.scope_id = native.sin6_scope_id;
				std::memcpy(out_end_point.address, &native.sin6_addr, 16);
				return true;
			}
			default:
				return false;
			}
		}

		//	Windows 版と揃え、1 回で扱える大きさを int の範囲に限る
		inline bool FitsInt(const std::size_t size) noexcept
		{
			return size <= static_cast<std::size_t>((std::numeric_limits<int>::max)());
		}

		Error SetIntOption(const Socket socket, const int level, const int name, const int value) noexcept
		{
			if (::setsockopt(ToNative(socket), level, name, &value, static_cast<::socklen_t>(sizeof(value))) != 0)
			{
				return LastError();
			}
			return Error::None;
		}

		[[maybe_unused]] inline void DisableInherit(const int socket) noexcept
		{
			const int flags = ::fcntl(socket, F_GETFD);
			if (flags >= 0)
			{
				::fcntl(socket, F_SETFD, flags | FD_CLOEXEC);
			}
		}

		inline void DisableSigPipe([[maybe_unused]] const int socket) noexcept
		{
#if defined(SO_NOSIGPIPE)
			const int value = 1;
			::setsockopt(socket, SOL_SOCKET, SO_NOSIGPIPE, &value, static_cast<::socklen_t>(sizeof(value)));
#endif // defined(SO_NOSIGPIPE)
		}

		//	recvmsg で受け、切り詰め (MSG_TRUNC) を Windows 版と同じく MessageTooLong で返す。
		//	recv / recvfrom では切り詰めを知る方法が OS ごとに違うため、受信はすべてここを通す。
		IoResult ReceiveMessage(const Socket socket, const std::span<nox::uint8> buffer, ::sockaddr_storage* const out_storage, const ReceiveFlag flags) noexcept
		{
			if (FitsInt(buffer.size()) == false)
			{
				return IoResult{ 0, Error::InvalidArgument };
			}

			::iovec io_vector;
			io_vector.iov_base = buffer.data();
			io_vector.iov_len = buffer.size();

			::msghdr message;
			std::memset(&message, 0, sizeof(message));
			message.msg_name = out_storage;
			message.msg_namelen = (out_storage != nullptr) ? static_cast<::socklen_t>(sizeof(::sockaddr_storage)) : 0;
			message.msg_iov = &io_vector;
			message.msg_iovlen = 1;

			::ssize_t result = 0;
			do
			{
				result = ::recvmsg(ToNative(socket), &message, ToNativeFlags(flags));
			}
			while (result < 0 && errno == EINTR);

			if (result < 0)
			{
				return IoResult{ 0, LastError() };
			}
			if ((message.msg_flags & MSG_TRUNC) != 0)
			{
				return IoResult{ static_cast<nox::int32>(result), Error::MessageTooLong };
			}
			return IoResult{ static_cast<nox::int32>(result), Error::None };
		}
	}

	Error Initialize() noexcept
	{
		return Error::None;
	}

	void Finalize() noexcept
	{
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
		int type = is_tcp ? SOCK_STREAM : SOCK_DGRAM;
#if defined(SOCK_CLOEXEC)
		type |= SOCK_CLOEXEC;
#endif // defined(SOCK_CLOEXEC)

		const int socket = ::socket(native_family, type, is_tcp ? IPPROTO_TCP : IPPROTO_UDP);
		if (socket < 0)
		{
			return LastError();
		}

#if !defined(SOCK_CLOEXEC)
		DisableInherit(socket);
#endif // !defined(SOCK_CLOEXEC)
		DisableSigPipe(socket);

		out_socket = FromNative(socket);
		return Error::None;
	}

	Error Close(Socket& socket) noexcept
	{
		if (socket.IsValid() == false)
		{
			return Error::None;
		}

		//	close は EINTR でもディスクリプタを解放済みの OS があるので、再試行しない
		const int result = ::close(ToNative(socket));
		socket = Socket{};
		return (result != 0 && errno != EINTR) ? LastError() : Error::None;
	}

	Error Bind(const Socket socket, const EndPoint& end_point) noexcept
	{
		NativeAddress address;
		if (ToNative(end_point, address) == false)
		{
			return Error::InvalidArgument;
		}

		if (::bind(ToNative(socket), reinterpret_cast<const ::sockaddr*>(&address.storage), address.length) != 0)
		{
			return LastError();
		}
		return Error::None;
	}

	Error Listen(const Socket socket, const nox::int32 backlog) noexcept
	{
		if (::listen(ToNative(socket), (backlog < 0) ? SOMAXCONN : backlog) != 0)
		{
			return LastError();
		}
		return Error::None;
	}

	Error Accept(const Socket listener, Socket& out_socket, EndPoint* const out_remote) noexcept
	{
		out_socket = Socket{};

		::sockaddr_storage storage;
		::socklen_t length = static_cast<::socklen_t>(sizeof(storage));
		int socket = -1;
		do
		{
#if defined(__linux__)
			socket = ::accept4(ToNative(listener), reinterpret_cast<::sockaddr*>(&storage), &length, SOCK_CLOEXEC);
#else
			socket = ::accept(ToNative(listener), reinterpret_cast<::sockaddr*>(&storage), &length);
#endif // defined(__linux__)
		}
		while (socket < 0 && errno == EINTR);

		if (socket < 0)
		{
			return LastError();
		}

#if !defined(__linux__)
		DisableInherit(socket);
#endif // !defined(__linux__)
		DisableSigPipe(socket);

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

		if (::connect(ToNative(socket), reinterpret_cast<const ::sockaddr*>(&address.storage), address.length) != 0)
		{
			//	EINTR で中断された connect は裏で続くので、再試行せず InProgress として扱う
			const Error error = LastError();
			return (error == Error::Interrupted) ? Error::InProgress : error;
		}
		return Error::None;
	}

	Error GetPendingError(const Socket socket, Error& out_error) noexcept
	{
		int value = 0;
		::socklen_t length = static_cast<::socklen_t>(sizeof(value));
		if (::getsockopt(ToNative(socket), SOL_SOCKET, SO_ERROR, &value, &length) != 0)
		{
			out_error = Error::None;
			return LastError();
		}

		out_error = ToError(value);
		return Error::None;
	}

	Error Shutdown(const Socket socket, const ShutdownMode mode) noexcept
	{
		int how = SHUT_RDWR;
		switch (mode)
		{
		case ShutdownMode::Receive:
			how = SHUT_RD;
			break;
		case ShutdownMode::Send:
			how = SHUT_WR;
			break;
		case ShutdownMode::Both:
			how = SHUT_RDWR;
			break;
		}

		if (::shutdown(ToNative(socket), how) != 0)
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

		::ssize_t result = 0;
		do
		{
			result = ::send(ToNative(socket), buffer.data(), buffer.size(), ToNativeFlags(flags));
		}
		while (result < 0 && errno == EINTR);

		if (result < 0)
		{
			return IoResult{ 0, LastError() };
		}
		return IoResult{ static_cast<nox::int32>(result), Error::None };
	}

	IoResult Receive(const Socket socket, const std::span<nox::uint8> buffer, const ReceiveFlag flags) noexcept
	{
		return ReceiveMessage(socket, buffer, nullptr, flags);
	}

	IoResult SendTo(const Socket socket, const std::span<const nox::uint8> buffer, const EndPoint& destination, const SendFlag flags) noexcept
	{
		NativeAddress address;
		if (FitsInt(buffer.size()) == false || ToNative(destination, address) == false)
		{
			return IoResult{ 0, Error::InvalidArgument };
		}

		::ssize_t result = 0;
		do
		{
			result = ::sendto(
				ToNative(socket),
				buffer.data(),
				buffer.size(),
				ToNativeFlags(flags),
				reinterpret_cast<const ::sockaddr*>(&address.storage),
				address.length);
		}
		while (result < 0 && errno == EINTR);

		if (result < 0)
		{
			return IoResult{ 0, LastError() };
		}
		return IoResult{ static_cast<nox::int32>(result), Error::None };
	}

	IoResult ReceiveFrom(const Socket socket, const std::span<nox::uint8> buffer, EndPoint* const out_source, const ReceiveFlag flags) noexcept
	{
		::sockaddr_storage storage;
		std::memset(&storage, 0, sizeof(storage));
		const IoResult result = ReceiveMessage(socket, buffer, &storage, flags);
		if (out_source != nullptr)
		{
			FromNative(storage, *out_source);
		}
		return result;
	}

	Error Poll(const std::span<PollEntry> entries, const nox::int32 timeout_milliseconds, nox::int32& out_ready_count) noexcept
	{
		out_ready_count = 0;
		if (entries.empty() == true || entries.size() > static_cast<std::size_t>(kMaxPollEntries))
		{
			return Error::InvalidArgument;
		}

		//	Windows 版と揃え、無効なソケットは poll に渡さずその場で Invalid を返す
		::pollfd native_entries[kMaxPollEntries];
		nox::uint8 native_to_entry[kMaxPollEntries];
		::nfds_t native_count = 0;
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

			::pollfd& native = native_entries[native_count];
			native.fd = ToNative(entry.socket);
			native.events = 0;
			native.revents = 0;
			if (HasAny(entry.requested, PollEvent::Readable))
			{
				native.events = static_cast<short>(native.events | POLLIN);
			}
			if (HasAny(entry.requested, PollEvent::Writable))
			{
				native.events = static_cast<short>(native.events | POLLOUT);
			}
			native_to_entry[native_count] = static_cast<nox::uint8>(i);
			++native_count;
		}

		if (native_count == 0)
		{
			out_ready_count = invalid_count;
			return Error::None;
		}

		//	EINTR は待ち時間を数え直す必要があるので、再試行せず Interrupted を返す
		const int result = ::poll(native_entries, native_count, (invalid_count > 0) ? 0 : timeout_milliseconds);
		if (result < 0)
		{
			return LastError();
		}

		for (::nfds_t i = 0; i < native_count; ++i)
		{
			const short revents = native_entries[i].revents;
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
		const int flags = ::fcntl(ToNative(socket), F_GETFL);
		if (flags < 0)
		{
			return LastError();
		}

		const int new_flags = enable ? (flags | O_NONBLOCK) : (flags & ~O_NONBLOCK);
		if (new_flags != flags && ::fcntl(ToNative(socket), F_SETFL, new_flags) != 0)
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
		::socklen_t length = static_cast<::socklen_t>(sizeof(storage));
		if (::getsockname(ToNative(socket), reinterpret_cast<::sockaddr*>(&storage), &length) != 0)
		{
			out_end_point = EndPoint{};
			return LastError();
		}
		return FromNative(storage, out_end_point) ? Error::None : Error::Unknown;
	}

	Error GetRemoteEndPoint(const Socket socket, EndPoint& out_end_point) noexcept
	{
		::sockaddr_storage storage;
		::socklen_t length = static_cast<::socklen_t>(sizeof(storage));
		if (::getpeername(ToNative(socket), reinterpret_cast<::sockaddr*>(&storage), &length) != 0)
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
#endif // defined(__unix__) || defined(__APPLE__)
