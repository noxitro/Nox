//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	network_detail.h
///	@brief	network_detail
#pragma once
#include	<cstddef>
#include	<limits>
#include	"../network.h"

namespace nox::os::network::detail
{
	///	@brief	1 回の送受信で扱える大きさか。Windows の send / recv は長さを int で受け取る
	inline constexpr bool FitsInt(const std::size_t value) noexcept
	{
		return value <= static_cast<std::size_t>((std::numeric_limits<nox::int32>::max)());
	}
}