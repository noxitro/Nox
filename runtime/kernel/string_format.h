//	Copyright (C) 2023 NOX ENGINE All Rights Rserved.

///	@file	string_format.h
///	@brief	string_format
#pragma once

#include	<format>
//#include	"convert_string.h"
#include	"unicode_converter.h"

#pragma warning(push, 0)
#pragma warning(disable: 26498)
//#pragma warning(disable:4365)
//#pragma warning(disable:4514)
//#pragma warning(disable:4582)
//#pragma warning(disable:4623)
//#pragma warning(disable:4625)
//#pragma warning(disable:4626)
//#pragma warning(disable:4710)
//#pragma warning(disable:4820)
//#pragma warning(disable:5027)
#include	"third_party/fmt/format.h"
#include	"third_party/fmt/xchar.h"
#pragma warning(pop)

#include	"algorithm.h"

namespace nox::util
{
	namespace detail
	{

		template<class CharType> requires(IsCharTypeValue<CharType>)
		struct FormatStringHolder
		{
			template<class From> requires(!std::is_same_v< CharType, From>)
			static inline nox::BasicString<CharType> Get(const From* arg)
			{
				return nox::unicode::ConvertString<nox::BasicString<CharType>>(arg);
			}

			template<class From> requires(!std::is_same_v< CharType, From>)
			static inline  nox::BasicString<CharType> Get(const nox::BasicString<From>& arg)
			{
				return nox::unicode::ConvertString<nox::BasicString<CharType>>(arg);
			}

			template<class From> requires(!std::is_same_v< CharType, From>)
				static inline  nox::BasicString<CharType> Get(const std::basic_string_view<From> arg)
			{
				return nox::unicode::ConvertString<nox::BasicString<CharType>>(arg);
			}

			//	span ver
			template<class From> //requires(!std::is_same_v< CharType, From>)
				static inline void Get(From&& arg, std::span< CharType> dest_buffer)
			{
				return nox::unicode::ConvertString<CharType>(std::forward<From>(arg), dest_buffer);
			}
		};

		//	formatの引数として渡すとき変換が必要かどうかをチェックする
		template<class To, class From>
		struct CheckThroughFormatString : std::false_type {};

		template<class To, class From> requires(std::is_arithmetic_v<std::decay_t<From>> && !IsCharTypeValue<std::decay_t<From>>)
			struct CheckThroughFormatString<To, From> : std::true_type {};

		template<class To, class From> requires(std::is_same_v<To, std::decay_t<std::remove_pointer_t<std::decay_t<From>>>>)
			struct CheckThroughFormatString<To, From> : std::true_type {};

		template<class To, class From> requires(IsStringClassAllValue<std::decay_t<From>> && std::is_same_v<To, typename std::decay_t<From>::value_type>)
			struct CheckThroughFormatString<To, From> : std::true_type {};

		template<class To, class From> requires(IsCharTypeValue<To>)
			constexpr bool CheckThroughFormatStringValue = CheckThroughFormatString<To, From>::value;

		template<class To, class From> requires(std::is_void_v<std::void_t<FormatStringHolder<To>>>)
		inline auto ToFormatArg(From&& arg)
		{
			if constexpr (CheckThroughFormatStringValue<To, From> == true)
			{
				return arg;
			}
			else
			{
				return FormatStringHolder<To>::Get(arg);
			}
			
		}

		/// @brief バッファ指定版
		/// @tparam To 
		/// @tparam From 
		/// @param arg 
		/// @param dest_buffer 
		/// @return 
		template<class To, class From> requires(std::is_void_v<std::void_t<FormatStringHolder<To>>>)
			inline auto ToFormatArg(From&& arg, std::span<To> dest_buffer)
		{
			if constexpr (CheckThroughFormatStringValue<To, From> == true)
			{
				return arg;
			}
			else
			{
				FormatStringHolder<To>::Get(arg, dest_buffer);
				return dest_buffer.data();
			}
		}
	}

	//template<class T, class... _Types>
	//inline	nox::BasicString<T>	FormatImpl(const T* const fmt, _Types&&... args)
	//{
	//	return util::detail::VFormat<T>(fmt, std::make_format_args<util::detail::BasicFormatContext<T>>(args...));
	//}

	//template<class... _Types>
	//inline nox::WString Format(const wchar16* const fmt, _Types&&... _Args)
	//{
	//	return util::FormatImpl(fmt, nox::util::detail::ToFormatArg<nox::WString>(_Args)...);
	//}

	//template<class... _Types>
	//inline nox::U16String Format(const char16* const fmt, _Types&&... _Args)
	//{
	//	nox::WString from_str = util::Format(reinterpret_cast<const wchar_t*>(fmt), _Args...);
	//	return nox::U16String(util::CharCast<const char16*>(from_str.c_str()), from_str.size());
	//}


	namespace detail
	{
		template <typename Char, size_t SIZE>
		[[nodiscard]] inline nox::BasicString<Char> FmtToString(const fmt::basic_memory_buffer<Char, SIZE>& buf)
		{
			auto size = buf.size();
			fmt::detail::assume(size < nox::BasicString<Char>().max_size());
			return nox::BasicString<Char>(buf.data(), size);
		}

		template <typename Char>
		inline nox::BasicString<Char> FmtVFormat(fmt::basic_string_view<Char> format_str,
			typename fmt::detail::vformat_args<Char>::type args)
		{
			auto buf = fmt::basic_memory_buffer<Char>();
			fmt::detail::vformat_to(buf, format_str, args);
			return nox::util::detail::FmtToString(buf);
		}

		template <typename StrType, class... Args>
		inline nox::BasicString<nox::StringCharType<StrType>> FormatImpl2(const StrType& format_str, const Args&... args)
		{
			return nox::util::detail::FmtVFormat(
				fmt::detail::to_string_view(format_str),
				::fmt::make_format_args<::fmt::buffered_context<nox::StringCharType<StrType>>>(args...));
		}

		//	span ver
		template<concepts::Char CharType, class... Args>
		inline void FormatImpl(std::span<CharType> dest_buffer, fmt::basic_string_view<CharType> format_str, Args&&... args)
		{
			fmt::basic_memory_buffer<CharType> buf;
			fmt::detail::vformat_to(buf, format_str, ::fmt::make_format_args<::fmt::buffered_context<CharType>>(args...));

			const size_t bufSize = buf.size();
			fmt::detail::assume(bufSize < nox::BasicString<CharType>().max_size());

			util::StrCopy({ buf.data(), bufSize }, dest_buffer);
		}

		template<concepts::Char CharType, class FormatStr, class ArgsTuple, size_t... Indices>
		inline void FormatImpl(std::span<CharType> dest_buffer, const FormatStr& format_str, ArgsTuple&& source, std::span<std::span<CharType>> dest_buffer_array, std::index_sequence<Indices...>)
		{
			FormatImpl(dest_buffer, fmt::detail::to_string_view(format_str), nox::util::detail::ToFormatArg<CharType>(std::get<Indices>(source), dest_buffer_array[Indices])...);
		}
	}

	/// @brief string format
	/// @tparam S 
	/// @tparam ...Args 
	/// @param format_str 
	/// @param ...args 
	/// @return 
	template <typename StrType, class... Args>
	inline nox::BasicString<nox::StringCharType<StrType>> Format(const StrType& format_str, Args&&... args)
	{
		return nox::util::detail::FormatImpl2(format_str, nox::util::detail::ToFormatArg< nox::StringCharType<StrType>>(std::forward<Args>(args))...);
	}

	/// @brief 指定バッファに格納するformat関数
	/// @tparam S フォーマット用文字列の型
	/// @tparam ...Args 引数群の型
	/// @tparam ArgumentBufferSize 引数用に確保する単体のバッファサイズ
	/// @param dest_buffer 結果を格納する出力用バッファ
	/// @param format_str フォーマット用文字列
	/// @param ...args 引数群
	template <class S, size_t ArgumentBufferSize = 128, class... Args>
	inline void Format(std::span<nox::StringCharType<S>> dest_buffer, const S& format_str, Args&&... args)
	{
		//	引数変換のための作業用バッファ
		std::array<std::array<nox::StringCharType<S>, ArgumentBufferSize>, sizeof...(Args)> args_buffer{ 0 };

		std::array<std::span<nox::StringCharType<S>>, sizeof...(Args)> args_span;
		for (size_t i = 0; i < args_buffer.size(); ++i)
		{
			args_span[i] = std::span<nox::StringCharType<S>>(args_buffer[i]);
		}

		util::detail::FormatImpl(dest_buffer, format_str, std::make_tuple(std::forward<Args>(args)...), std::span<std::span<nox::StringCharType<S>>>(args_span), std::make_index_sequence<sizeof...(Args)>());
	}
}