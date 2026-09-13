// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

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