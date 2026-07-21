// Copyright (C) 2026 NOX ENGINE All rights reserved.

/// @file	mesh_asset.h
/// @brief	mesh_asset
#pragma once

namespace nox::render
{
	class MeshAsset final : public nox::Asset
	{
		NOX_DECLARE_OBJECT(MeshAsset, nox::Asset);
	public:

	private:
		bool OnInitialize(nox::io::BinaryReader& reader) override;
	};
}