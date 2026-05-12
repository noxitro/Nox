//	Copyright (c) 2025 NOX ENGINE All rights reserved.

///	@file	scene_resource.cpp
///	@brief	scene_resource
#include	"pch.h"
#include	"scene_resource.h"

namespace nox
{
	struct ComponentNativeData
	{
		nox::char32 fqn[256];

	};

	struct GameObjectNativeData
	{
		nox::char32 name[256];
		nox::Guid id;

		nox::uint16 component_count;
		ComponentNativeData component_list[1];
	};

	struct SceneNativeData
	{
		nox::char32 name[256];
		nox::Guid id;

		nox::int16 object_count;
		GameObjectNativeData object_list[1];
	};
}

void nox::SceneResource::OnInitialize(const nox::io::Stream& stream) 
{
	
}