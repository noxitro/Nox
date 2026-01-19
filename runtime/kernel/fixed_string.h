//	Copyright (C) 2024 NOX ENGINE All Rights Rserved.

///	@file	fixed_string.h
///	@brief	fixed_string
#pragma once
#include	"type_traits/type_traits.h"
#include	"memory/memory_util.h"
#include    "math/math_algorithm.h"

namespace nox
{
    template<class T, size_t _Length> requires(_Length >= 1)
        struct BasicFixedString
    {
    public:
        using value_type = T;
        static constexpr size_t Length = _Length;

        constexpr BasicFixedString() noexcept :
            array_{},
            native_length_(0U)
        {
        }

        inline constexpr BasicFixedString(const T(&s)[_Length]) noexcept
        {
            native_length_ = _Length;
            std::ranges::copy_n(s, native_length_, array_.begin());
        }

        // 格納可能な最大文字数（終端を管理しない設計なら _Length）
        inline constexpr size_t Capacity() const noexcept { return _Length; }

        [[nodiscard]] inline constexpr size_t Size() const noexcept { return native_length_; }
        inline constexpr std::span<const T> AsSpan() const noexcept { return std::span<const T>(array_.data(), native_length_); }
        inline constexpr const std::array<T, _Length>& AsArray() const noexcept { return array_; }

        inline constexpr void Assign(std::basic_string_view<T> str)
        {
            native_length_ = static_cast<uint32_t>(nox::math::Min(str.length(), static_cast<size_t>(_Length)));
            std::ranges::copy_n(str.data(), native_length_, array_.begin());
        }

        inline constexpr BasicFixedString& operator=(std::basic_string_view<T> s) noexcept
        {
            native_length_ = static_cast<uint32_t>(nox::math::Min(s.length(), static_cast<size_t>(_Length)));
            std::ranges::copy_n(s.data(), native_length_, array_.begin());
            return *this;
        }

        inline constexpr operator std::basic_string_view<T>() const noexcept
        {
            return std::basic_string_view<T>{ array_.data(), native_length_};
        }

        inline constexpr operator std::span<T>() noexcept
        {
            return std::span<T>(array_.data(), native_length_);
        }

        inline constexpr operator std::span<const T>()const noexcept 
        { 
            return std::span<const T>(array_.data(), native_length_); 
        }

        inline constexpr bool operator==(std::basic_string_view<T> other) const noexcept
        {
            const std::basic_string_view<T> a = *this;
			return a == other;
        }

    private:
        std::array<T, _Length> array_;
        nox::uint32 native_length_;
    };
}