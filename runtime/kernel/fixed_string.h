//	Copyright (C) 2024 NOX ENGINE All Rights Rserved.

///	@file	fixed_string.h
///	@brief	fixed_string
#pragma once
#include	"type_traits/type_traits.h"
#include	"memory/memory_util.h"
#include    "math/math_algorithm.h"

namespace nox
{
    namespace detail
    {
        void CheckStringLength(std::size_t length, std::size_t maxLength);
    }

    template<class T, size_t _Length> requires(_Length >= 1)
        struct BasicFixedString
    {
    public:
        using value_type = T;
        static constexpr size_t Length = _Length;

        inline constexpr BasicFixedString() noexcept :
            array_{}
        {
        }

        inline constexpr BasicFixedString(std::basic_string_view<T> s ):
            array_{}
        {
            if (s.length() > _Length)
            {
                nox::detail::CheckStringLength(s.length(), _Length);
            }
            std::ranges::copy_n(s.data(), s.length(), array_.begin());
        }

        inline constexpr BasicFixedString(const T(&s)[_Length]) noexcept
        {
            std::ranges::copy_n(s, _Length, array_.begin());
        }

        inline constexpr size_t Capacity() const noexcept { return _Length; }

        [[nodiscard]] inline constexpr size_t Size() const noexcept
        {
            return std::char_traits<T>::length(array_.data());
        }
        inline constexpr std::span<const T> AsSpan() const noexcept { return std::span<const T>(array_.data(), Size()); }
        inline constexpr const std::array<T, _Length>& AsArray() const noexcept { return array_; }

        inline constexpr void Assign(std::basic_string_view<T> str)
        {
            nox::uint32 length = static_cast<nox::uint32>(nox::math::Min(str.length(), static_cast<size_t>(_Length)));
            std::ranges::copy_n(str.data(), length, array_.begin());
        }

        inline constexpr BasicFixedString& operator=(std::basic_string_view<T> s) noexcept
        {
            nox::uint32 length = static_cast<nox::uint32>(nox::math::Min(s.length(), static_cast<size_t>(_Length)));
            std::ranges::copy_n(s.data(), length, array_.begin());
            return *this;
        }

        inline constexpr operator std::basic_string_view<T>() const noexcept
        {
            return std::basic_string_view<T>{ array_.data(), Size()};
        }

        inline constexpr operator std::span<T>() noexcept
        {
            return std::span<T>(array_.data(), Size());
        }

        inline constexpr operator std::span<const T>()const noexcept 
        { 
            return std::span<const T>(array_.data(), Size()); 
        }

        inline constexpr bool operator==(std::basic_string_view<T> other) const noexcept
        {
            const std::basic_string_view<T> a = *this;
			return a == other;
        }

    private:
        std::array<T, _Length> array_;
    };

	template<size_t _Length>
	using U8FixedString = BasicFixedString<char8, _Length>;
}