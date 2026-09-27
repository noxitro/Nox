//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	string.h
///	@brief	string
#pragma once
#include	"advanced_type.h"
#include	"basic_definition.h"
#include	"ascii.h"

#include	"nox_string_view.h"

namespace nox
{
	template<class T>
	class BasicString
	{
	public:
		using string_type = nox::StlBasicString<T>;
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

	public:
		inline constexpr BasicString()noexcept {}
		inline constexpr BasicString(const BasicString&) = default;
		inline constexpr BasicString(BasicString&& other) noexcept :
			string_(std::move(other.string_)) {
		}

		template<class U> requires (std::constructible_from<string_type, U&&>)
		inline constexpr BasicString(U&& other) noexcept(noexcept(string_type(std::forward<U>(other)))) :
			string_(std::forward<U>(other)) {
		}

		inline constexpr explicit BasicString(const value_type* s, size_type count) noexcept(noexcept(string_(s, count))) :
			string_(s, count) {
		}

		inline	constexpr	size_t	Capacity()const noexcept(noexcept(string_.capacity())) { return string_.capacity(); }
		inline	constexpr void* Data()noexcept(noexcept(string_.data())) { return string_.data(); }
		inline	constexpr const void* Data()const noexcept(noexcept(string_.data())) { return string_.data(); }

		inline constexpr	const T* CStr()const noexcept(noexcept(string_.c_str())) { return string_.c_str(); }

		inline	constexpr	void	Clear()noexcept(noexcept(string_.clear())) { string_.clear(); }
		inline	constexpr	void	ShrinkToFit() { string_.shrink_to_fit(); }

		inline	constexpr	void Resize(size_type new_size) { string_.resize(new_size); }

		inline	BasicString& Append(const std::basic_string_view<T> s)
		{
			string_.append(s);
			return *this;
		}

		inline	BasicString& Append(T s)
		{
			string_.push_back(s);
			return *this;
		}

		inline constexpr size_type Find(T s, nox::uint32 offset = 0)const noexcept
		{
			return string_.find(s, offset);
		}

#pragma region operator
		inline constexpr explicit operator string_type& () noexcept { return string_; }
		inline constexpr explicit operator const string_type& ()const noexcept { return string_; }

		inline constexpr operator std::basic_string_view<T>() const noexcept {
			return string_;
		}

		inline constexpr explicit operator nox::BasicStringView<T>() const noexcept(noexcept(nox::BasicStringView<T>(string_))) {
			return nox::BasicStringView<T>(string_);
		}

		inline constexpr BasicString& operator =(const BasicString&) = default;

		[[nodiscard]]
		inline	constexpr bool operator ==(const BasicString& rhs) const noexcept
		{
			return string_ == rhs.string_;
		}

		[[nodiscard]]
		inline	constexpr const_reference operator [](size_type index) const noexcept { return string_[index]; }

		inline BasicString& operator +=(const std::basic_string_view<T> s)
		{
			this->Append(s);
			return *this;
		}

		inline BasicString& operator +=(T s)
		{
			this->Append(s);
			return *this;
		}

#pragma endregion

#pragma region convert
		
		/// @brief unicode文字列変換
		/// @tparam To 
		/// @return 
		template<class To> requires(std::is_same_v<To, T> == false)
		inline nox::BasicString<To>	ToUString()const
		{
			nox::BasicString<To> result;
			return result;
		}

		/// @brief ascii文字列変換
		/// @tparam To 
		/// @return 
		template<class To> requires(std::is_same_v<To, T> == false)
		inline constexpr nox::BasicString<To>	ToAsciiString()const
		{
			nox::BasicString<To> result;
			result.Resize(string_.size());
			nox::encoding::ascii::ConvertString<To, T>(std::basic_string_view<T>(string_), std::span<To>(result.string_.data(), result.string_.size()));
			return result;
		}
#pragma endregion

	private:
		string_type string_;
	};

	using U8String = nox::BasicString<nox::char8>;
	using U16String = nox::BasicString<nox::char16>;
	using U32String = nox::BasicString<nox::char32>;

//
//	namespace detail
//	{
//		template<class _StringType>
//		class StringBase
//		{
//		public:
//			using string_type = _StringType;
//			using traits_type = typename string_type::traits_type;
//			using allocator_type = typename string_type::allocator_type;
//			using value_type = typename string_type::value_type;
//			using size_type = typename string_type::size_type;
//			using difference_type = typename string_type::difference_type;
//			using pointer = typename string_type::pointer;
//			using const_pointer = typename string_type::const_pointer;
//			using reference = typename string_type::reference;
//			using const_reference = typename string_type::const_reference;
//			using iterator = typename string_type::iterator;
//			using const_iterator = typename string_type::const_iterator;
//			using reverse_iterator = typename string_type::reverse_iterator;
//			using const_reverse_iterator = typename string_type::const_reverse_iterator;
//
//
//		protected:
//
//		};
//	}
//	
//	/// @brief String
//	class String : public nox::detail::StringBase<nox::StlU16String>
//	{
//	public:
//		inline constexpr	String() noexcept :
//			string_() {}
//
//		inline constexpr String(const String& other) :
//			string_(other.string_) {}
//
//		inline constexpr String(String&& other)noexcept :
//			string_(std::move(other.string_)) {}
//
//		inline constexpr String(nox::StlBasicString<value_type>&& other)noexcept :
//			string_(std::move(other)) {}
//
//		String(class nox::StringView other)noexcept;
//
//		inline constexpr explicit String(const string_type& other) :
//			string_(other) {}
//
//		inline constexpr explicit String(std::basic_string_view<value_type> other) :
//			string_(other) {}
//
//		inline constexpr explicit String(const value_type* s) :
//			string_(s) {}
//
//		inline constexpr explicit String(const value_type* s, size_type count) :
//			string_(s, count) {}
//
//#pragma region 関数
//
//		inline	constexpr	size_t	Capacity()const noexcept { return string_.capacity(); }
//		inline	constexpr not_null<const void*> Data()const noexcept { return string_.data(); }
//		inline constexpr	not_null<const value_type*> CStr()const noexcept { return string_.c_str(); }
//
//		inline	constexpr	void	Clear()noexcept { string_.clear(); }
//		inline	constexpr	void	ShrinkToFit() { string_.shrink_to_fit(); }
//
//		inline	String& Append(const String& s)
//		{
//			string_.append(s.string_);
//			return *this;
//		}
//
//		inline	String& Append(value_type s)
//		{
//			string_.push_back(s);
//			return *this;
//		}
//
//		inline String& Append(const value_type* s)
//		{
//			string_.append(s);
//			return *this;
//		}
//
//		inline	String& Append(std::basic_string_view<value_type> s)
//		{
//			string_.append(s);
//			return *this;
//		}
//
//	//	inline	void	Swap(String& other)noexcept { string_.swap(other.string_); }
//#pragma endregion
//
//#pragma region iterator
//		inline	constexpr string_type::iterator begin()noexcept { return string_.begin(); }
//		inline	constexpr	string_type::iterator end()noexcept { return string_.end(); }
//
//		inline	constexpr string_type::reverse_iterator rbegin()noexcept { return string_.rbegin(); }
//		inline	constexpr	string_type::reverse_iterator rend()noexcept { return string_.rend(); }
//
//		inline	constexpr	string_type::const_iterator begin()const noexcept { return string_.begin(); }
//		inline	constexpr	string_type::const_iterator end()const noexcept { return string_.end(); }
//
//		inline	constexpr	string_type::const_iterator cbegin()const noexcept { return string_.cbegin(); }
//		inline	constexpr	string_type::const_iterator cend()const noexcept { return string_.cend(); }
//
//		inline	constexpr string_type::const_reverse_iterator crbegin()noexcept { return string_.crbegin(); }
//		inline	constexpr	string_type::const_reverse_iterator crend()noexcept { return string_.crend(); }
//#pragma endregion
//
//#pragma region operator
//		inline String& operator +=(const String& s)
//		{
//			Append(s);
//			return *this;
//		}
//
//		inline String& operator +=(const nox::StlBasicString<value_type>& s)
//		{
//			Append(s);
//			return *this;
//		}
//
//		inline String& operator +=(value_type s)
//		{
//			Append(s);
//			return *this;
//		}
//
//		inline String& operator +=(const value_type* s)
//		{
//			Append(s);
//			return *this;
//		}
//
//		inline	String& operator +=(const std::basic_string_view<value_type> s)
//		{
//			Append(s);
//			return *this;
//		}
//
//		[[nodiscard]] inline constexpr operator nox::StlU16String& () noexcept { return string_; }
//		[[nodiscard]] inline constexpr operator const nox::StlU16String& ()const noexcept { return string_; }
//#pragma endregion
//
//
//#pragma region 変換
//		nox::StlNString ToNString()const;
//
//		nox::StlWString ToWString()const;
//
//		nox::StlU32String	ToU32String()const;
//#pragma endregion
//	private:
//		string_type string_;
//	};
}