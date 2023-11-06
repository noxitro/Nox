//	Copyright (C) 2023 NOX ENGINE All Rights Rserved.

///	@file	string_format.h
///	@brief	string_format
#pragma once

#include	<format>
#include	"convert_string.h"

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
	}

	template<class T, class... _Types>
	inline	nox::BasicString<T>	Format(const T* const fmt, _Types&&... args)
	{
		return util::detail::VFormat<T>(fmt, std::make_format_args<util::detail::BasicFormatContext<T>>(args...));
	}

	template<class... _Types>
	inline nox::U16String Format(const c16* const fmt, _Types&&... _Args)
	{
		nox::WString from_str = util::Format(reinterpret_cast<const wchar_t*>(fmt), util::CastStringSafe<const w16*>(_Args)...);
		return nox::U16String(util::CharCast<const c16*>(from_str.c_str()));
	}
}