//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	fixed_vector.h
///	@brief	fixed_vector
#pragma once
#include	"advanced_type.h"
#include	"assertion_kernel.h"

namespace nox
{
	template<class T, nox::uint32 _Size>
	class FixedVector
	{
	public:
		using value_type = T;
		static constexpr nox::uint32 Size = _Size;
	public:
		inline constexpr FixedVector()noexcept :
			storage_{},
			length_(0)
		{
		}

		inline constexpr explicit FixedVector(std::span<T> v) :
			storage_(Make(v)),
			length_(static_cast<nox::uint32>(v.size()))
		{
		}
		
		inline constexpr FixedVector(FixedVector&& other)noexcept :
			storage_(std::move(other.storage_)),
			length_(other.length_)
		{
			other.storage_ = {};
			other.length_ = 0;
		}

		inline constexpr FixedVector& operator=(FixedVector&& other)noexcept
		{
			if (this != &other)
			{
				storage_ = std::move(other.storage_);
				length_ = other.length_;
				other.storage_ = {};
				other.length_ = 0;
			}
			return *this;
		}

		inline constexpr void PushBack(const T& value)
		{
			CheckOverflow();
			storage_[length_++] = value;
		}

		inline constexpr void PushBack(T&& value)
		{
			CheckOverflow();
			storage_[length_++] = std::move(value);
		}

		inline void Assign(std::span<T> v)
		{
			storage_ = Make(v);
			length_ = static_cast<nox::uint32>(v.size());
		}

		inline constexpr operator std::span<T>() noexcept
		{
			return std::span<T>(storage_.data(), length_);
		}

		inline constexpr operator std::span<const T>() const noexcept
		{
			return std::span<const T>(storage_.data(), length_);
		}

		inline constexpr const std::array<T, _Size>& GetStorage()const noexcept { return storage_; }
		inline constexpr nox::uint32 GetLength()const noexcept { return length_; }

	private:
		inline constexpr std::array<T, _Size> Make(std::span<T> v)
		{
			std::array<T, _Size> result{};
			NOX_ASSERT_KERNEL(v.size() <= Size, u"FixedVectorのサイズがオーバーフローしました");
			const nox::uint32 copy_count = static_cast<nox::uint32>(v.size());
			std::ranges::copy_n(v.begin(), copy_count, result.begin());
			return result;
		}

		inline void CheckOverflow()const
		{
			NOX_ASSERT_KERNEL(length_ <= Size, u"FixedVectorのサイズがオーバーフローしました");
		}
	private:
		std::array<T, _Size> storage_;
		nox::uint32 length_;
	};
}