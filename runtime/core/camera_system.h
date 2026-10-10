//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	camera_system.h
///	@brief	camera_system
#pragma once
#include	"entity_system.h"
#include	"component.h"
#include	"attribute_common.h"
#include	"attribute_dev_common.h"

namespace nox::components
{
	struct CameraOutput;
	/// @brief カメラの設定情報
	struct NOX_ATTR(nox::attr::RequireComponents<CameraOutput>())
		CameraSettings final : public nox::Component<CameraSettings>
	{
		nox::float32 fov;
		nox::float32 aspect_ratio;
		nox::float32 near;
		nox::float32 far;
	};

	/// @brief カメラの出力情報
	struct 
		NOX_ATTR_TYPE(nox::attr::RequireComponents<CameraSettings>())
		CameraOutput final : public nox::Component<CameraOutput>
	{
		nox::Mat4 view_matrix;
		nox::Mat4 projection_matrix;
	};
}

namespace nox::systems
{
	struct Camera final : public nox::EntitySystem<Camera>
	{
		NOX_ECS_DECLARE_VERIFY(Camera);
		
		static void OnUpdate(
			nox::Entity entity, 
			nox::components::CameraOutput& output, 
			const nox::components::CameraSettings& settings
		)
		{

		}
	};
}