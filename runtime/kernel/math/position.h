// Copyright (C) 2026 NOX ENGINE All rights reserved.

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

		inline constexpr Position(const nox::Vec3& v) noexcept :
			x(v.x), y(v.y), z(v.z) {
		}

		inline constexpr nox::Vec3 ToVec3() const noexcept { return nox::Vec3(static_cast<nox::float_t>(x), static_cast<nox::float_t>(y), static_cast<nox::float_t>(z)); }
	};

	static_assert(std::is_trivially_copyable_v<nox::Position>);
}