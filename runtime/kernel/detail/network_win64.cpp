//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	network_win64.cpp
///	@brief	network_win64
#include	"pch.h"
#include	"../win64_socket.h"
#if NOX_WINDOWS
#include	<optional>
#include	<cstring>
#include	"network_detail.h"
#include	"../utility.h"
#include	"../algorithm.h"
#include	"../memory/memory_util.h"

namespace nox::os::network
{
	namespace
	{
		static_assert(
			static_cast<nox::intptr>(INVALID_SOCKET) == nox::os::network::Socket::kInvalidSocket, 
			"INVALID_SOCKETはintptrにすると-1になるはず");

		inline constexpr int GetNativeFamily(nox::os::network::IpFamily family)noexcept
		{
			switch (family)
			{
			case nox::os::network::IpFamily::IPv4: return AF_INET;
			case nox::os::network::IpFamily::IPv6: return AF_INET6;
			default: return AF_UNSPEC;
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
			case WSAEINPROGRESS:
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
				addr.sin_port = ::htons(static_cast<::u_short>(end_point.port));
				std::memcpy(&addr.sin_addr, end_point.address, 4);
				result.length = sizeof(::sockaddr_in);
			}
			break;
			case nox::os::network::IpFamily::IPv6:
			{
				auto& addr = reinterpret_cast<::sockaddr_in6&>(result.storage);
				addr.sin6_family = AF_INET6;
				addr.sin6_port = ::htons(static_cast<::u_short>(end_point.port));
				addr.sin6_scope_id = end_point.scope_id;
				std::memcpy(&addr.sin6_addr, end_point.address, 16);
				result.length = sizeof(::sockaddr_in6);
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
				if (length < sizeof(::sockaddr_in))
				{
					return std::nullopt;
				}
				const auto& addr = reinterpret_cast<const ::sockaddr_in&>(storage);
				result.family = nox::os::network::IpFamily::IPv4;
				result.port = static_cast<nox::os::network::Port>(::ntohs(addr.sin_port));
				std::memcpy(result.address, &addr.sin_addr, 4);
			}
			break;
			case AF_INET6:
			{
				if (length < sizeof(::sockaddr_in6))
				{
					return std::nullopt;
				}
				const auto& addr = reinterpret_cast<const ::sockaddr_in6&>(storage);
				result.family = nox::os::network::IpFamily::IPv6;
				result.port = static_cast<nox::os::network::Port>(::ntohs(addr.sin6_port));
				result.scope_id = addr.sin6_scope_id;
				std::memcpy(result.address, &addr.sin6_addr, 16);
			}
			break;
			default:
				return std::nullopt;
			}
			return result;
		}

		inline nox::os::network::Error SetSocketOption(nox::os::network::Socket socket, int level, int option_name, const int value)noexcept
		{
			const int result = ::setsockopt(ToNative(socket), level, option_name, reinterpret_cast<const char*>(&value), sizeof(value));
			if (result == SOCKET_ERROR)
			{
				return nox::os::network::LastError();
			}
			return nox::os::network::Error::None;
		}

		inline void DisableInherit(const ::SOCKET socket)noexcept
		{
			::SetHandleInformation(reinterpret_cast<::HANDLE>(socket), HANDLE_FLAG_INHERIT, 0);
		}

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
	}
}

nox::os::network::Error nox::os::network::Initialize()noexcept
{
	::WSADATA wsa_data{};
	return nox::os::network::ToError(::WSAStartup(MAKEWORD(2, 2), &wsa_data));
}

void nox::os::network::Finalize()noexcept
{
	::WSACleanup();
}

std::expected<nox::os::network::Socket, nox::os::network::Error> nox::os::network::CreateSocket(nox::os::network::IpFamily family, nox::os::network::Protocol protocol)noexcept
{
	const int native_family = nox::os::network::GetNativeFamily(family);
	if (native_family == AF_UNSPEC)
	{
		return std::unexpected(nox::os::network::Error::InvalidArgument);
	}

	const bool is_tcp = (protocol == nox::os::network::Protocol::TCP);
	const ::SOCKET socket = ::WSASocketW(
		native_family,
		is_tcp ? SOCK_STREAM : SOCK_DGRAM,
		is_tcp ? ::IPPROTO_TCP : ::IPPROTO_UDP,
		nullptr,
		0,
		WSA_FLAG_OVERLAPPED | WSA_FLAG_NO_HANDLE_INHERIT
	);

	if (socket == INVALID_SOCKET)
	{
		return std::unexpected(nox::os::network::LastError());
	}

	return nox::os::network::FromNative(socket);
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

nox::os::network::Error nox::os::network::Listen(const nox::os::network::Socket socket, const int backlog)noexcept
{
	if (::listen(nox::os::network::ToNative(socket), backlog) == SOCKET_ERROR)
	{
		return nox::os::network::LastError();
	}
	return nox::os::network::Error::None;
}

std::expected<nox::os::network::AcceptedSocket, nox::os::network::Error> nox::os::network::Accept(nox::os::network::Socket listener) noexcept
{
	::accept
	(
		nox::os::network::ToNative(listener),
		nullptr,
		nullptr
	);
}

std::expected<nox::os::network::EndPoint, nox::os::network::Error> nox::os::network::GetLocalEndPoint(nox::os::network::Socket socket) noexcept;
std::expected<nox::os::network::EndPoint, nox::os::network::Error> nox::os::network::GetRemoteEndPoint(nox::os::network::Socket socket) noexcept;
#endif // NOX_WINDOWS