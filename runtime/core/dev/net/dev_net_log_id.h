//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

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