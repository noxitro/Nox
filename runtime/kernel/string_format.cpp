//	Copyright (C) 2023 NOX ENGINE All Rights Rserved.

///	@file	string_format.cpp
///	@brief	string_format
#include	"pch.h"
#include	"string_format.h"

namespace nox
{
	constinit size_t k_string_max_size_char = []()constexpr {
		return std::basic_string<char>().max_size();
		}(); 
	constinit size_t k_string_max_size_wchar = []()constexpr {
		return std::basic_string<wchar_t>().max_size();
		}();

	constinit size_t k_string_max_size_char8 = []()constexpr {
		return std::basic_string<char8>().max_size();
		}();
	constinit size_t k_string_max_size_char16 = []()constexpr {
		return std::basic_string<char16>().max_size();
		}();
	constinit size_t k_string_max_size_char32 = []()constexpr {
		return std::basic_string<char32>().max_size();
		}();

	
}

	template<>
	size_t nox::util::detail::GetStringMaxSize<char>()
	{
		return k_string_max_size_char;
	}

	template<>
	size_t nox::util::detail::GetStringMaxSize<wchar_t>()
	{
		return k_string_max_size_wchar;
	}

	template<>
	size_t nox::util::detail::GetStringMaxSize<nox::char8>()
	{
		return k_string_max_size_char8;
	}

	template<>
	size_t nox::util::detail::GetStringMaxSize<nox::char16>()
	{
		return k_string_max_size_char16;
	}

	template<>
	size_t nox::util::detail::GetStringMaxSize<nox::char32>()
	{
		return k_string_max_size_char32;
	}

