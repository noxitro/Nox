// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

/// @file	asset.h
/// @brief	asset
#pragma once
#include	"object.h"

namespace nox
{
	class AssetChunkLoader;
	class Asset : public nox::Object
	{
		NOX_DECLARE_OBJECT(Asset, nox::Object);
	public:
		inline constexpr Asset() : is_initialized_(false) {}
		virtual ~Asset()override {}

		inline constexpr bool IsReady()const noexcept { return is_initialized_; }

		bool Initialize(const nox::AssetChunkLoader& loader);
	protected:
		virtual bool OnInitialize(const nox::AssetChunkLoader& loader) = 0;
	private:
		bool is_initialized_;
	};
}