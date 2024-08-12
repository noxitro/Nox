//	Copyright (C) 2024 NOX ENGINE All Rights Rserved.

///	@file	fixed_string.h
///	@brief	fixed_string
#pragma once
#include	"type_traits/type_traits.h"
#include	"memory/memory_util.h"

namespace nox
{
	template<class T, size_t _Length> //requires(nox::IsCharTypeValue<_T>)
	class BasicFixedString
	{
	public:
		using value_type = T;
		static constexpr size_t Length = _Length;

		consteval BasicFixedString()noexcept :
			str_array_{ T() }
			, native_length_(0U)
		{
		}

		inline	constexpr	BasicFixedString(std::span<T> s)noexcept:
			str_array_(s.data()),
			native_length_(s.size())
		{

		}

#pragma region 関数

		/**
		 * @brief 文字列アドレスを取得
		 * @return 文字列アドレス
		*/
		[[nodiscard]] inline constexpr const T* const CStr()const noexcept { return static_cast<const T* const>(str_array_.data()); }
		[[nodiscard]] inline constexpr T* CStr()noexcept { return static_cast<T*>(str_array_.data()); }

		/**
		 * @brief
		 * @return
		*/
		[[nodiscard]] inline constexpr const void* const Data()const noexcept { return static_cast<const void* const>(str_array_.data()); }
		[[nodiscard]] inline constexpr void* const Data()noexcept { return static_cast<void* const>(str_array_.data()); }

		/**
		 * @brief 文字列をセット
		 * @param ary
		 * @return
		*/
		template<size_t Len> requires(Len <= _Length)
			inline	constexpr void Set(const T(&ary)[Len])noexcept
		{
			set(ary, std::make_index_sequence<Len>());
		}

		inline	void	Append(std::basic_string_view<value_type> s)
		{
			
		}

#pragma endregion

	private:
		std::array<T, _Length> str_array_;

		/// @brief 実際の文字列長
		uint32 native_length_;
	};
}