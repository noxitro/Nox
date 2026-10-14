//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	network.cpp
///	@brief	network の OS に依らない部分。OS ごとの実装は detail/network_<os>.cpp
#include	"pch.h"
#include	"network.h"

const char16_t* nox::os::network::ToString(const Error error) noexcept
{
	switch (error)
	{
	case Error::None: return u"None";
	case Error::WouldBlock: return u"WouldBlock";
	case Error::InProgress: return u"InProgress";
	case Error::Interrupted: return u"Interrupted";
	case Error::TimedOut: return u"TimedOut";
	case Error::ConnectionReset: return u"ConnectionReset";
	case Error::ConnectionRefused: return u"ConnectionRefused";
	case Error::ConnectionAborted: return u"ConnectionAborted";
	case Error::NotConnected: return u"NotConnected";
	case Error::AddressInUse: return u"AddressInUse";
	case Error::AddressNotAvailable: return u"AddressNotAvailable";
	case Error::NetworkUnreachable: return u"NetworkUnreachable";
	case Error::HostUnreachable: return u"HostUnreachable";
	case Error::MessageTooLong: return u"MessageTooLong";
	case Error::InvalidArgument: return u"InvalidArgument";
	case Error::NoResources: return u"NoResources";
	case Error::AccessDenied: return u"AccessDenied";
	case Error::NotInitialized: return u"NotInitialized";
	case Error::Unknown: return u"Unknown";
	}
	return u"Unknown";
}
