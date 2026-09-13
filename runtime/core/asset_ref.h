// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

/// @file	asset_ref.h
/// @brief	asset_ref
#pragma once

namespace nox
{
	class Asset;

	/// @brief		アセット参照情報
	/// @details	Editorとの同期は、この型をAssetとして扱います。Assetポインタは不可能で、AssetRefを使用してください。
	/// @note		
	template<std::derived_from<nox::Asset> T>
	struct AssetRef
	{
		inline constexpr const nox::Asset* Get() const noexcept { return asset_; }
	private:
		T* asset_;
	};
}