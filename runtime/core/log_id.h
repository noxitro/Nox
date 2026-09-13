//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	log_id.h
///	@brief	log_id
#pragma once

namespace nox::log_id
{
	struct CoreCommon : public nox::log_id::LogId
	{
		inline constexpr std::u8string_view operator()() const noexcept { return u8"CoreCommon"; }
	};

	struct Resource : public nox::log_id::LogId
	{
		inline constexpr std::u8string_view operator()() const noexcept { return u8"Resource"; }
	};
}