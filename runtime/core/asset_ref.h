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
			constexpr AssetHandleBase()noexcept : guid_(nox::Guid::Empty()) {}

			inline constexpr nox::Guid GetGuid()const noexcept { return guid_; }
			inline constexpr bool IsValid()const noexcept { return guid_ != nox::Guid::Empty(); }

		private:
			NOX_ATTR_DECLARE(nox::attr::DataMember())
			nox::Guid guid_;
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