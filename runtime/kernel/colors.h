// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

/// @file	colors.h
/// @brief	colors
#pragma once
#include	"color.h"

namespace nox::colors
{
	constexpr nox::Color kRed{ 255U, 0U, 0U, 255U };
	constexpr nox::Color kGreen{ 0U, 255U, 0U, 255U };
	constexpr nox::Color kBlue{ 0U, 0U, 255U, 255U };
	constexpr nox::Color kYellow{ 255U, 255U, 0U, 255U };
	constexpr nox::Color kCyan{ 0U, 255U, 255U, 255U };
	constexpr nox::Color kMagenta{ 255U, 0U, 255U, 255U };
	constexpr nox::Color kWhite{ 255U, 255U, 255U, 255U };
	constexpr nox::Color kBlack{ 0U, 0U, 0U, 255U };
}