//	Copyright (C) 2024 NOX ENGINE All Rights Rserved.

///	@file	nox_string_view.h
///	@brief	nox_string_view
#pragma once
#include	"advanced_type.h"
#include	"unicode_converter.h"
namespace nox
{
	namespace detail
	{
		template<class _StringViewType>
		class StringViewBase
		{
		public:
			using string_view_type = _StringViewType;
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
		};
	}

	/// @brief	文字列の所有権を保持せず、文字列のコピーを持つのではなく参照をして
	///			参照先の文字列を加工して扱うクラス
	class StringView //: public nox::detail::StringViewBase<std::u32string_view>
	{
	public:
		using string_view_type = std::u32string_view;
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

	public:
			inline constexpr StringView()noexcept = default;

			inline	constexpr StringView(const StringView&)noexcept = default;

			inline StringView(const BasicString<value_type>& s) noexcept:
				view_(s) {}

			StringView(const class String& s)noexcept;

			inline	constexpr StringView(std::u32string_view s) noexcept:
				view_(s){}

			inline	constexpr StringView(const value_type* s, size_type length) noexcept:
				view_(s, length){}

			inline	constexpr StringView(const value_type* s) noexcept:
				view_(s){}


#pragma region 関数
		inline	std::span<char16>	ToU16String(std::span<char16> dest_buffer)const
		{
			return unicode::ConvertU16String(view_, dest_buffer);
		}

		inline	U16String	ToU16String()const { return unicode::ConvertU16String(view_); }
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

#pragma region operator
		inline constexpr StringView& operator =(const StringView&) = default;

		[[nodiscard]]
		inline	constexpr std::strong_ordering operator <=>(const StringView& rhs) const noexcept = default;
		[[nodiscard]]
		inline	constexpr const_reference operator [](size_type index) const noexcept { return view_[index]; }

		inline constexpr operator std::u32string_view() const noexcept { return view_; }
#pragma endregion

	private:
		string_view_type view_;
	};
}