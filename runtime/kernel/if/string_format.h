//	Copyright (C) 2023 NOX ENGINE All Rights Rserved.

///	@file	string_format.h
///	@brief	string_format
#pragma once

#include	<format>
//#include	"convert_string.h"
#include	"unicode_converter.h"

namespace nox::util
{
	namespace detail
	{
		template<class T>
		using BasicFormatIt = std::back_insert_iterator<std::_Fmt_buffer<T>>;

		template<class T>
		using BasicFormatContext = std::basic_format_context<BasicFormatIt<T>, T>;

		template<class T>
		using BasicFormatArgs = std::basic_format_args<BasicFormatContext<T>>;

		template <class T>
		inline nox::BasicString<T> VFormat(const std::basic_string_view<T> _Fmt, const BasicFormatArgs<T> _Args) {
			nox::BasicString<T> _Str;
			_Str.reserve(_Fmt.size() + _Args._Estimate_required_capacity());
			std::vformat_to(std::back_insert_iterator{ _Str }, _Fmt, _Args);
			return _Str;
		}

		template<class To>
		struct FormatStringHolder;

		template<class CharaType> 
		struct FormatStringHolder<nox::BasicString<CharaType>>
		{
			template<class From> requires(!std::is_same_v< CharaType, From>)
			static inline nox::BasicString<CharaType> Get(const From* arg)
			{
				return nox::unicode::ConvertString<nox::BasicString<CharaType>>(arg);
			}

			template<class From> requires(!std::is_same_v< CharaType, From>)
			static inline  nox::BasicString<CharaType> Get(const nox::BasicString<From>& arg)
			{
				return nox::unicode::ConvertString<nox::BasicString<CharaType>>(arg);
			}

			template<class From> requires(!std::is_same_v< CharaType, From>)
				static inline  nox::BasicString<CharaType> Get(const std::basic_string_view<From> arg)
			{
				return nox::unicode::ConvertString<nox::BasicString<CharaType>>(arg);
			}

			template<class From> requires(std::is_arithmetic_v<From>)
			static constexpr inline  decltype(auto) Get(From arg)noexcept
			{
				return arg;
			}

			template<class From> requires(std::is_same_v< CharaType, From>)
				static constexpr inline  decltype(auto) Get(const From* arg)noexcept
			{
				return arg;
			}

			template<class From> requires(std::is_same_v< CharaType, From>)
				static constexpr inline  decltype(auto) Get(const nox::BasicString<From>& arg)noexcept
			{
				return arg;
			}

			template<class From> requires(std::is_same_v< CharaType, From>)
				static constexpr inline  decltype(auto) Get(const std::basic_string_view<From> arg)noexcept
			{
				return arg;
			}

			
		};

		template<class To, class From>
		inline decltype(auto) ToFormatArg(From&& arg)
		{
			return FormatStringHolder<To>::Get(arg);
		}

	}

	template<class T, class... _Types>
	inline	nox::BasicString<T>	FormatImpl(const T* const fmt, _Types&&... args)
	{
		return util::detail::VFormat<T>(fmt, std::make_format_args<util::detail::BasicFormatContext<T>>(args...));
	}

	template<class... _Types>
	inline nox::WString Format(const wchar16* const fmt, _Types&&... _Args)
	{
		return util::FormatImpl(fmt, nox::util::detail::ToFormatArg<nox::WString>(_Args)...);
	}

	template<class... _Types>
	inline nox::U16String Format(const char16* const fmt, _Types&&... _Args)
	{
		nox::WString from_str = util::Format(reinterpret_cast<const wchar_t*>(fmt), _Args...);
		return nox::U16String(util::CharCast<const char16*>(from_str.c_str()), from_str.size());
	}

	
}