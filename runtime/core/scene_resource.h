//	Copyright (C) 2025 NOX ENGINE All rights reserved.

///	@file	scene_resource.h
///	@brief	scene_resource
#pragma once
#include "resource.h"
#include "attribute_common.h"
namespace nox
{
	/// @brief SceneResource
	class NOX_ATTR_TYPE(nox::attr::Resource(u8"scn", 0)) 
		SceneResource : public nox::Resource
	{
		NOX_DECLARE_OBJECT(SceneResource, nox::Resource);
	public:

	private:
		void onInitialize(const nox::io::Stream& stream) override;

	private:
		
	};
}