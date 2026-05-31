// Copyright (C) 2026 NOX ENGINE All rights reserved.

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
		Studio = 1 << 0,
		Win64 = 1 << 1,
		Android = 1 << 2,
	};

	namespace os
	{
		nox::PlatformType GetPlatformType()noexcept;

		inline constexpr std::u8string_view GetPlatformTypeName(nox::PlatformType type)noexcept
		{
			switch (type)
			{
			case nox::PlatformType::None:
				return u8"None";
			case nox::PlatformType::Studio:
				return u8"Studio";
			case nox::PlatformType::Win64:
				return u8"Win64";
			case nox::PlatformType::Android:
				return u8"Android";
			}
		}
	}
}