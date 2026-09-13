// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

///	@file	vector3d.cpp
///	@brief	vector3d
#include	"pch.h"
#include	"vector3d.h"

#include	"../string_format.h"
#include	"../nox_string.h"

namespace nox::detail
{
	namespace
	{
		template<std::floating_point T>
		inline std::u8string_view ToStringVector3D(std::span<nox::char8> dest, const nox::detail::Vector3D<T>& value)
		{
			nox::util::Format(dest, u8"({},{},{})", value.x, value.y, value.z);
			return std::u8string_view(dest.data());
		}
	}

	template<concepts::Arithmetic T>
	[[nodiscard]] inline constexpr std::optional<T> Parse(std::string_view str) noexcept
	{
		T value{};
		const auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), value);
		if (ec == std::errc{})
		{
			return value;
		}
		return std::nullopt;
	}

}

template<>
std::u8string_view nox::detail::Vector3D<nox::float_t>::ToString(std::span<nox::char8> dest)const noexcept
{
	return nox::detail::ToStringVector3D(dest, *this);
}

template<>
std::u8string_view nox::detail::Vector3D<nox::double_t>::ToString(std::span<nox::char8> dest)const noexcept
{
	return nox::detail::ToStringVector3D(dest, *this);
}

template<>
nox::U8String nox::detail::Vector3D<nox::float_t>::ToString()const
{
	return nox::util::Format(u8"({},{},{})", x, y, z);
}

template<>
nox::U8String nox::detail::Vector3D<nox::double_t>::ToString()const
{
	return nox::util::Format(u8"({},{},{})", x, y, z);
}

template<>
nox::detail::Vector3D<nox::float_t> nox::detail::Vector3D<nox::float_t>::FromString(std::u8string_view str)noexcept
{
	nox::detail::Vector3D<nox::float_t> result;

	return result;
}

template<>
nox::detail::Vector3D<nox::double_t> nox::detail::Vector3D<nox::double_t>::FromString(std::u8string_view str)noexcept
{
	nox::detail::Vector3D<nox::double_t> result;

	return result;
}