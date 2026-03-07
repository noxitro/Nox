//	Copyright (C) 2024 NOX ENGINE All Rights Rserved.

///	@file	nox_string_view.h
///	@brief	nox_string_view
#pragma once
#include	"advanced_type.h"
#include	"ascii.h"

namespace nox
{
	/// @brief	文字列の所有権を保持せず、文字列のコピーを持つのではなく参照をして
	///			参照先の文字列を加工して扱うクラス
	/// @tparam T 文字型
	template<class T>
	class BasicStringView final
	{
	public:
		using string_view_type = std::basic_string_view<T>;
		using traits_type = typename string_view_type::traits_type;
		using value_type = typename string_view_type::value_type;
		using pointer = typename string_view_type::pointer;
		using const_pointer = typename string_view_type::const_pointer;
		using reference = typename string_view_type::reference;
		using const_reference = typename string_view_type::const_reference;
		using const_iterator = typename string_view_type::const_iterator;
		using iterator = typename string_view_type::iterator;
		using const_reverse_iterator = typename string_view_type::const_reverse_iterator;
		using reverse_iterator = typename string_view_type::reverse_iterator;
		using size_type = typename string_view_type::size_type;
		using difference_type = typename string_view_type::difference_type;

		static constexpr size_type npos{ static_cast<size_type>(-1) };
		static constexpr nox::uint32 k_default_convert_buffer_size = 256;
	public:
		inline constexpr BasicStringView()noexcept :
			view_{} {
		}

		inline constexpr BasicStringView(const BasicStringView&)noexcept = default;
		inline constexpr BasicStringView(BasicStringView&& other)noexcept :
			view_(std::move(other.view_)) {
		}

		template<class U> requires (std::constructible_from<string_view_type, U&&>)
			inline	constexpr BasicStringView(U&& other) noexcept :
			view_(std::forward<U>(other)) {
		}

#pragma region operator
		inline constexpr operator string_view_type() const noexcept {
			return view_;
		}

		inline constexpr BasicStringView& operator =(const BasicStringView&) = default;

		[[nodiscard]]
		inline	constexpr std::strong_ordering operator <=>(const BasicStringView& rhs) const noexcept = default;
		[[nodiscard]]
		inline	constexpr const_reference operator [](size_type index) const noexcept { return view_[index]; }
#pragma endregion


#pragma region iterator

		[[nodiscard]]
		inline	constexpr const_iterator begin() const noexcept { return view_.begin(); }

		[[nodiscard]]
		inline	constexpr const_iterator end() const noexcept { return view_.end(); }

		[[nodiscard]]
		inline	constexpr const_iterator cbegin() const noexcept { return view_.cbegin(); }

		[[nodiscard]]
		inline	constexpr const_iterator cend() const noexcept { return view_.cend(); }

		[[nodiscard]]
		inline	constexpr const_reverse_iterator rbegin() const noexcept { return view_.rbegin(); }

		[[nodiscard]]
		inline	constexpr const_reverse_iterator rend() const noexcept { return view_.rend(); }

		[[nodiscard]]
		inline	constexpr const_reverse_iterator crbegin() const noexcept { return view_.crbegin(); }

		[[nodiscard]]
		inline	constexpr const_reverse_iterator crend() const noexcept { return view_.crend(); }


		[[nodiscard]]
		inline	constexpr const_reference at(size_type index) const { return view_.at(index); }

		[[nodiscard]]
		inline	constexpr const_reference front() const noexcept { return view_.front(); }

		[[nodiscard]]
		inline	constexpr const_reference back() const noexcept { return view_.back(); }

		[[nodiscard]]
		inline	constexpr const_pointer data() const noexcept { return view_.data(); }

		[[nodiscard]]
		inline	constexpr size_type size() const noexcept { return view_.size(); }

		[[nodiscard]]
		inline	constexpr size_type length() const noexcept { return view_.length(); }

		[[nodiscard]]
		inline	constexpr bool empty() const noexcept { return view_.empty(); }
#pragma endregion

#pragma region convert
		/// @brief 十分なバッファでConvertAsciiを呼び出す
		/// @tparam To 
		/// @param dest_buffer 
		/// @return 
		template<class To, size_t convert_buffer_size = k_default_convert_buffer_size>
		inline constexpr std::array<To, convert_buffer_size> ConvertAscii()const noexcept
		{
			std::array<To, convert_buffer_size> buffer{To()};
			nox::encoding::ascii::ConvertString<To>(view_, std::span<To>(buffer.data(), buffer.size()));
			return buffer;
		}

		template<typename To, size_t convert_buffer_size = k_default_convert_buffer_size>
		inline constexpr std::optional<std::array<To, convert_buffer_size>> TryConvertAscii()const noexcept
		{
			std::array<To, convert_buffer_size> buffer{ To() };
			const auto converted = nox::encoding::ascii::TryConvertString<To>(view_, std::span<To>(buffer.data(), buffer.size()));
			if (converted.has_value() == false)
			{
				return std::nullopt;
			}
			return buffer;
		}

		template<class To>
		inline constexpr nox::BasicStringView<To> ConvertAscii(std::span<To> dest_buffer)const noexcept
		{
			const auto converted = nox::encoding::ascii::ConvertString<To>(view_, dest_buffer);
			return nox::BasicStringView<To>(converted);
		}

		template<class To>
		inline constexpr std::optional<nox::BasicStringView<To>> TryConvertAscii(std::span<To> dest_buffer)const noexcept
		{
			const auto converted = nox::encoding::ascii::TryConvertString<To>(view_, dest_buffer);
			if (converted.has_value() == false)
			{
				return std::nullopt;
			}
			return nox::BasicStringView<To>(converted.value());
		}
#pragma endregion

#pragma region convert utf
		inline std::u8string_view ToUTF8(std::span<nox::char8> buffer) const requires(!std::is_same_v<T, nox::char8>)
		{
			return this->ToUTF8Impl(buffer);
		}
#pragma endregion

	private:
		std::u8string_view ToUTF8Impl(std::span<nox::char8> buffer) const;


	private:
		string_view_type view_;
	};

	using U8StringView = nox::BasicStringView<nox::char8>;
	using U16StringView = nox::BasicStringView<nox::char16>;
	using U32StringView = nox::BasicStringView<nox::char32>;
	using WStringView = nox::BasicStringView<nox::wchar16>;
}