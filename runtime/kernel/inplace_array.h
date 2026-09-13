//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	inplace_array.h
///	@brief	inplace_array
#pragma once
#include	"advanced_type.h"

namespace nox
{
	template<class T, size_t _Size>
	class InplaceArray
	{
	public:
		using value_type = T;

		inline constexpr InplaceArray() noexcept :
			data_{}
			, length_(0)
		{
		}

		inline constexpr explicit InplaceArray(const size_t length) noexcept :
			data_{}
			, length_(length)
		{
		}


		inline constexpr ~InplaceArray()noexcept
		{
		}

		inline constexpr std::span<T> AsSpan() noexcept
		{
			return std::span<T>(data_.data(), length_);
		}

		inline constexpr std::span<const T> AsSpan()const noexcept
		{
			return std::span<const T>(data_.data(), length_);
		}

	private:
		std::array<T, _Size> data_;
		size_t length_;
	};
}