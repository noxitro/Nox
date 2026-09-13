// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

/// @file	builtin_entities.h
/// @brief	builtin_entities
#pragma once
#include	"entity.h"

namespace nox::entities
{
	namespace detail
	{
		enum class BuiltinEntityId : nox::uint8
		{
			Invalid,
		};
	}

	constexpr nox::EntityId kInvalidEntityId{ 0u };
}