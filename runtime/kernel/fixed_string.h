//	Copyright (C) 2024 NOX ENGINE All Rights Rserved.

///	@file	fixed_string.h
///	@brief	fixed_string
#pragma once
#include	"type_traits/type_traits.h"
#include	"memory/memory_util.h"

namespace nox
{
    template<class T, size_t _Length> requires(_Length >= 1)
    class BasicFixedString
    {
    public:
        using value_type = T;
        static constexpr size_t Length = _Length;

        consteval BasicFixedString() noexcept :
            array_{ },
            native_length_(0U)
        {
        }

        inline constexpr BasicFixedString(const T(&s)[_Length]) noexcept :
            native_length_(_Length-1)
        {
            std::ranges::copy(s, array_.begin());
        }

#pragma region 関数

        /**
         * @brief 文字列アドレスを取得
         * @return 文字列アドレス
        */
        [[nodiscard]] inline constexpr const T* const CStr() const noexcept { return static_cast<const T* const>(array_.data()); }
        [[nodiscard]] inline constexpr T* CStr() noexcept { return static_cast<T*>(array_.data()); }

        /**
         * @brief
         * @return
        */
        [[nodiscard]] inline constexpr const void* Data() const noexcept { return static_cast<const void* const>(array_.data()); }
        [[nodiscard]] inline constexpr void* Data() noexcept { return static_cast<void*>(array_.data()); }

		inline constexpr size_t Capacity() const noexcept { return _Length; }

        /**
         * @brief 文字列の長さを取得
         * @return 文字列の長さ
        */
        [[nodiscard]] inline constexpr size_t Size() const noexcept { return native_length_; }


		inline constexpr std::span<const T> ToSpan() const noexcept { return std::span<const T>(array_.data(), native_length_); }

#pragma endregion

    private:

        std::array<T, _Length> array_;

        /// @brief 実際の文字列長
        uint32_t native_length_;
    };

    template<size_t _Length>
    class U32FixedString : public BasicFixedString<char32_t, _Length>
    {
    public:
        inline constexpr U32FixedString(const char32_t(&s)[_Length]) noexcept:
            BasicFixedString<char32_t, _Length>(s)
        {
        }
    };
}