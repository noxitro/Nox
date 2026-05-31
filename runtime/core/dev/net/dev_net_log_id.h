//	Copyright (C) 2025 NOX ENGINE All rights reserved.

///	@file	dev_net_log_id.h
///	@brief	dev_net_log_id
#pragma once

namespace nox::dev::net::log_id
{
	struct DevNet : nox::log_id::LogId
	{
		inline constexpr std::u8string_view operator()() const noexcept { return u8"DevNet"; }
	};
}