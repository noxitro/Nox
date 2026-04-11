//	Copyright (c) 2025 NOX ENGINE All rights reserved.

#include	"pch.h"
#if NOX_DEVELOP
#include	"system.g.h"
#include	"../../application.h"
#include	"../../scene_manager.h"
#include	"../../scene_view.h"

nox::PlacementObject<nox::dev::editor_remote::Response> nox::dev::editor_remote::ResourceConvertQuery::Execute(std::span<nox::uint8> storage)const
{
	return nullptr;
}

nox::PlacementObject<nox::dev::editor_remote::Response> nox::dev::editor_remote::GetMainSceneView::Execute(std::span<nox::uint8> storage)const
{
	auto scene_view_info = nox::PlacementObject<nox::dev::editor_remote::SceneViewInfo>::Construct(storage);							
	nox::SceneManager& scene_manager = nox::SceneManager::Instance();
	auto& scene_view = scene_manager.GetMainSceneView();
	auto window_handle = scene_view.GetWindow().GetNativeHandle();

	scene_view_info->SetSceneView(&scene_view);
	scene_view_info->SetMainWindowHandle(reinterpret_cast<nox::intptr>(window_handle));
	return scene_view_info;
}
#endif // NOX_DEVELOP