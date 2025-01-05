//	Copyright (C) 2024 NOX ENGINE All Rights Rserved.

///	@file	string.h
///	@brief	string
#pragma once

#include	"advanced_type.h"
#include	"unicode_converter.h"
namespace nox
{
	//	前方宣言
	class StringView;

	namespace detail
	{
		template<class _StringType>
		class StringBase
		{
		public:
			using string_type = _StringType;
			using traits_type = typename string_type::traits_type;
			using allocator_type = typename string_type::allocator_type;
			using value_type = typename string_type::value_type;
			using size_type = typename string_type::size_type;
			using difference_type = typename string_type::difference_type;
			using pointer = typename string_type::pointer;
			using const_pointer = typename string_type::const_pointer;
			using reference = typename string_type::reference;
			using const_reference = typename string_type::const_reference;
			using iterator = typename string_type::iterator;
			using const_iterator = typename string_type::const_iterator;
			using reverse_iterator = typename string_type::reverse_iterator;
			using const_reverse_iterator = typename string_type::const_reverse_iterator;


		protected:

		};
	}

	/// @brief String
	class String : public nox::detail::StringBase<nox::U32String>
	{
	public:
		inline constexpr	String() noexcept :
			string_() {}

		inline constexpr String(const String& other) :
			string_(other.string_) {}

		inline constexpr String(String&& other)noexcept :
			string_(std::move(other.string_)) {}

		inline constexpr String(nox::BasicString<value_type>&& other)noexcept :
			string_(std::move(other)) {}

		String(class nox::StringView other)noexcept;

		inline constexpr explicit String(const string_type& other) :
			string_(other) {}

		inline constexpr explicit String(std::basic_string_view<value_type> other) :
			string_(other) {}

		inline constexpr explicit String(const value_type* s) :
			string_(s) {}

		inline constexpr explicit String(const value_type* s, size_type count) :
			string_(s, count) {}

#pragma region 関数

		inline	constexpr	size_t	Capacity()const noexcept { return string_.capacity(); }
		inline	constexpr not_null<const void*> Data()const noexcept { return string_.data(); }
		inline constexpr	not_null<const value_type*> CStr()const noexcept { return string_.c_str(); }

		inline	constexpr	void	Clear()noexcept { string_.clear(); }
		inline	constexpr	void	ShrinkToFit() { string_.shrink_to_fit(); }

		inline	String& Append(const String& s)
		{
			string_.append(s.string_);
			return *this;
		}

		inline	String& Append(value_type s)
		{
			string_.push_back(s);
			return *this;
		}

		inline String& Append(const value_type* s)
		{
			string_.append(s);
			return *this;
		}

		inline	String& Append(std::basic_string_view<value_type> s)
		{
			string_.append(s);
			return *this;
		}

	//	inline	void	Swap(String& other)noexcept { string_.swap(other.string_); }
#pragma endregion

#pragma region iterator
		inline	constexpr string_type::iterator begin()noexcept { return string_.begin(); }
		inline	constexpr	string_type::iterator end()noexcept { return string_.end(); }

		inline	constexpr string_type::reverse_iterator rbegin()noexcept { return string_.rbegin(); }
		inline	constexpr	string_type::reverse_iterator rend()noexcept { return string_.rend(); }

		inline	constexpr	string_type::const_iterator begin()const noexcept { return string_.begin(); }
		inline	constexpr	string_type::const_iterator end()const noexcept { return string_.end(); }

		inline	constexpr	string_type::const_iterator cbegin()const noexcept { return string_.cbegin(); }
		inline	constexpr	string_type::const_iterator cend()const noexcept { return string_.cend(); }

		inline	constexpr string_type::const_reverse_iterator crbegin()noexcept { return string_.crbegin(); }
		inline	constexpr	string_type::const_reverse_iterator crend()noexcept { return string_.crend(); }
#pragma endregion

#pragma region operator
		inline String& operator +=(const String& s)
		{
			Append(s);
			return *this;
		}

		inline String& operator +=(const nox::BasicString<value_type>& s)
		{
			Append(s);
			return *this;
		}

		inline String& operator +=(value_type s)
		{
			Append(s);
			return *this;
		}

		inline String& operator +=(const value_type* s)
		{
			Append(s);
			return *this;
		}

		inline	String& operator +=(const std::basic_string_view<value_type> s)
		{
			Append(s);
			return *this;
		}

		[[nodiscard]] inline constexpr operator nox::U32String& () noexcept { return string_; }
		[[nodiscard]] inline constexpr operator const nox::U32String& ()const noexcept { return string_; }
#pragma endregion


#pragma region 変換
		inline nox::NString ToNString()const
		{
			return unicode::ConvertNString(string_);
		}

		inline nox::WString ToWString()const
		{
			return unicode::ConvertWString(string_);
		}

		inline	nox::U16String	ToU16String()const
		{
			return unicode::ConvertU16String(string_);
		}
#pragma endregion
	private:
		string_type string_;
	};
}