// Copyright (C) 2026 NOX ENGINE All rights reserved.

/// @file	scene_asset.h
/// @brief	scene_asset
#pragma once
#include	"asset.h"

namespace nox
{
	class SceneAsset final : public nox::Asset
	{
		NOX_DECLARE_OBJECT(SceneAsset, nox::Asset);
	public:

	private:
		bool OnInitialize(nox::io::BinaryReader& reader) override;
	};
}