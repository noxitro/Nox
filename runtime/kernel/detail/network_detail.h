//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	network_detail.h
///	@brief	network_detail
#pragma once
#include	"../network.h"

namespace nox::os::network
{
	inline constexpr bool FitsInt(const nox::uint64 value) noexcept
	{
		return value <= static_cast<nox::uint64>(std::numeric_limits<nox::int32>::max());
	}
}