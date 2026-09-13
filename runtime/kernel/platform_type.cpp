// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

/// @file	platform_type.cpp
/// @brief	platform_type
#include "pch.h"
#include "platform_type.h"

#include	"algorithm.h"

nox::PlatformType nox::os::GetPlatformType()noexcept
{
#if NOX_WIN64
	return nox::PlatformType::Win64;
#endif // NOX_WIN64

#if NOX_ANDROID
	 return nox::PlatformType::Android;
#endif // NOX_ANDROID

	return nox::PlatformType::None;
}