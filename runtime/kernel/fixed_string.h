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

		template<size_t _OtherLength> requires(_OtherLength <= _Length && _OtherLength != _Length)
        inline constexpr BasicFixedString(const T(&s)[_OtherLength]) noexcept :
            native_length_(_OtherLength - 1)
        {
            std::ranges::copy(s, array_.begin());
        }

#pragma region 関数

        [[nodiscard]] inline constexpr const T* const CStr() const noexcept { return static_cast<const T* const>(array_.data()); }
        [[nodiscard]] inline constexpr T* CStr() noexcept { return static_cast<T*>(array_.data()); }
        [[nodiscard]] inline constexpr const void* Data() const noexcept { return static_cast<const void* const>(array_.data()); }
        [[nodiscard]] inline constexpr void* Data() noexcept { return static_cast<void*>(array_.data()); }

		inline constexpr size_t Capacity() const noexcept { return _Length; }

        /**
         * @brief 文字列の長さを取得
         * @return 文字列の長さ
        */
        [[nodiscard]] inline constexpr size_t Size() const noexcept { return native_length_; }


		inline constexpr std::span<const T> AsSpan() const noexcept { return std::span<const T>(array_.data(), native_length_); }
		inline constexpr const std::array<T, _Length>& AsArray() const noexcept { return array_; }
#pragma endregion
		template<size_t _OtherLength> requires(_OtherLength < _Length)
        inline constexpr BasicFixedString& operator=(const T(&s)[_OtherLength]) noexcept
        {
            native_length_ = _OtherLength - 1;
            std::ranges::copy(s, array_.begin());
            return *this;
		}

        inline constexpr BasicFixedString& operator=(std::basic_string_view<T> s) noexcept
        {
            native_length_ = static_cast<uint32_t>(nox::math::Min(s.length(), static_cast<size_t>(_Length - 1)));
            std::ranges::copy_n(s.data(), native_length_, array_.begin());
			return *this;
        }

        inline constexpr operator std::basic_string_view<T>() const noexcept
        {
            return std::basic_string_view<T>{ array_.data(), native_length_};
		}

        inline constexpr bool operator==(const BasicFixedString<T, _Length>& other) const noexcept
        {
            if (native_length_ != other.native_length_)
            {
                return false;
            }

            if consteval
            {
                // 定数評価時は constexpr に評価可能なループを使う
                for (size_t i = 0; i < native_length_; ++i)
                {
                    if (array_[i] != other.array_[i])
                    {
                        return false;
                    }
                }
                return true;
            }
            else 
            {
                const std::size_t bytes = static_cast<std::size_t>(native_length_) * sizeof(T);
                return std::memcmp(static_cast<const void*>(array_.data()),
                    static_cast<const void*>(other.array_.data()),
                    bytes) == 0;
            }
		}
 //   private:

        std::array<T, _Length> array_;

        /// @brief 実際の文字列長
        uint32_t native_length_;
    };
}