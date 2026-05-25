// Copyright (C) 2026 NOX ENGINE All rights reserved.

/// @file	local_transform.h
/// @brief	local_transform
#pragma once
#include	"component.h"

namespace nox
{
	struct LocalTransform : IComponentData
	{
		nox::Position position;
		nox::Quat rotation;
		nox::Vec3 scale;
	};
}