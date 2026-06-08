// Copyright (C) 2026 NOX ENGINE All rights reserved.

/// @file	asset_attribute.h
/// @brief	asset_attribute
#pragma once
#include	"attribute.h"

namespace nox::attr
{
	class Asset : public nox::attr::Attribute
	{
		NOX_DECLARE_OBJECT(Asset, nox::attr::Attribute);
	public:
		inline constexpr explicit Asset(std::u8string_view extension, nox::uint32 version)noexcept :
			extension_(extension),
			version_(version)
		{
		}

		inline constexpr std::u8string_view GetExtension()const noexcept { return extension_; }
		inline constexpr nox::uint32 GetVersion()const noexcept { return version_; }

	private:
		/// @brief 拡張子
		std::u8string_view extension_;

		/// @brief リソースバージョン
		nox::uint32 version_;
	};
}