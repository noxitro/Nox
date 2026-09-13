// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

/// @file	platform_type.h
/// @brief	platform_type
#pragma once
#include	"basic_definition.h"
#include	"advanced_type.h"

namespace nox
{
	enum class PlatformType : nox::uint16
	{
		None,
		Win64,
		Android,
	};

	namespace os
	{
		nox::PlatformType GetPlatformType()noexcept;

		inline constexpr std::u8string_view GetPlatformTypeName(nox::PlatformType type)noexcept
		{
			switch (type)
			{
			case nox::PlatformType::Win64:	return u8"Win64";
			case nox::PlatformType::Android:	return u8"Android";
			}
			return u8"None";
		}
	}
}