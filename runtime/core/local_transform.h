// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

/// @file	local_transform.h
/// @brief	local_transform
#pragma once
#include	"component.h"

namespace nox
{
	struct NOX_ATTR_TYPE(nox::reflection::attr::Reflection()) LocalTransform : IComponentData
	{
		nox::Position position;
		nox::Quat rotation;
		nox::Vec3 scale;
	};
}