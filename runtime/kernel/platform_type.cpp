// Copyright (C) 2026 NOX ENGINE All rights reserved.

/// @file	platform_type.cpp
/// @brief	platform_type
#include "pch.h"
#include "platform_type.h"

#include	"algorithm.h"

nox::PlatformType nox::os::GetPlatformType()noexcept
{
	nox::PlatformType result = nox::PlatformType::None;
	 const auto proc = [&result](nox::PlatformType type)constexpr noexcept
		{
			 result = nox::util::BitOr(result, type);
		};

#if NOX_DEVELOP
	 proc(nox::PlatformType::Studio);
#endif // NOX_DEVELOP

#if NOX_WIN64
	 proc(nox::PlatformType::Win64);
#endif // NOX_WIN64

#if NOX_ANDROID
	 proc(nox::PlatformType::Android);
#endif // NOX_ANDROID

	return result;
}