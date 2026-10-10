// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

/// @file	common_components.h
/// @brief	common_components
#pragma once
#include	"component.h"
#include	"ecs_definitions.h"
#include	"entity.h"

namespace nox::components
{
	/// @brief 親子関係を持つEntityの親Entityを示すComponent
	struct Parent final : public nox::Component<Parent>
	{
		NOX_ECS_DECLARE_VERIFY(Parent);
		nox::Entity value;
	};

	struct LocalPosition final : public nox::Component<LocalPosition>
	{
		NOX_ECS_DECLARE_VERIFY(LocalPosition);
		nox::Position value;
	};

	struct LocalRotation final : public nox::Component<LocalRotation>
	{
		NOX_ECS_DECLARE_VERIFY(LocalRotation);
		nox::Quat value;
	};

	struct LocalScale final : public nox::Component<LocalScale>
	{
		NOX_ECS_DECLARE_VERIFY(LocalScale);
		nox::Vec3 value;
	};
}