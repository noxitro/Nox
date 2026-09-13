// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

/// @file	position.h
/// @brief	position
#pragma once
#include	"../basic_type.h"
#include	"vector3d.h"

namespace nox
{
	struct Position
	{
		nox::double_t x;
		nox::double_t y;
		nox::double_t z;

		inline constexpr Position() noexcept :
			x(0.0), y(0.0), z(0.0) {
		}

		inline constexpr explicit Position(nox::double_t _x, nox::double_t _y, nox::double_t _z) noexcept :
			x(_x), y(_y), z(_z) {
		}

		inline constexpr explicit Position(const nox::Vec3& v) noexcept :
			x(v.x), y(v.y), z(v.z) {
		}

		inline constexpr nox::Vec3 ToVec3() const noexcept { return nox::Vec3(static_cast<nox::float_t>(x), static_cast<nox::float_t>(y), static_cast<nox::float_t>(z)); }
		inline static constexpr Position Zero()noexcept { return Position(); }
	};

	static_assert(std::is_trivially_copyable_v<nox::Position>);
}