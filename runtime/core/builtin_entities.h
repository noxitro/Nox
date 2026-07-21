// Copyright (C) 2026 NOX ENGINE All rights reserved.

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

	constexpr nox::EntityId kInvalidEntityId = nox::EntityId{ 0u, 0u };
}