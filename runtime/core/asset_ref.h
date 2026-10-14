// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

/// @file	asset_ref.h
/// @brief	asset_ref
#pragma once
#include	"attribute_common.h"

namespace nox
{
	class Asset;

	namespace detail
	{
		struct AssetHandleBase
		{
			constexpr AssetHandleBase()noexcept :
				asset_id_(0) {}

			inline constexpr nox::uint64 GetAssetId()const noexcept { return asset_id_; }
			inline constexpr bool IsValid()const noexcept { return asset_id_ != 0; }

		private:
			NOX_ATTR_DECLARE(nox::attr::DataMember())
			nox::uint64 asset_id_;
		};
	}

	/// @brief		アセット参照情報
	/// @details	Editorとの同期は、この型をAssetとして扱います。Assetポインタは不可能で、AssetRefを使用してください。
	/// @note		
	template<std::derived_from<nox::Asset> T>
	struct AssetHandle final : public nox::detail::AssetHandleBase
	{
		
	};
}