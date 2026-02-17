//	Copyright (C) 2026 NOX ENGINE All rights reserved.

///	@file	parse.h
///	@brief	parse
#pragma once
#include	<charconv>
#include	<string_view>
#include	<optional>
#include	"type_traits/concepts.h"

namespace nox::parse
{
	// 前方宣言
	template<nox::concepts::Arithmetic T>
	[[nodiscard]] inline constexpr std::optional<T> Parse(std::string_view str) noexcept;

	namespace detail
	{
		/// @brief 先頭の空白をスキップする
		[[nodiscard]] inline constexpr std::string_view TrimLeft(std::string_view str) noexcept
		{
			size_t i = 0;
			while (i < str.size() && str[i] == ' ') ++i;
			return str.substr(i);
		}

		/// @brief カンマ区切りの次のトークンを取り出す
		[[nodiscard]] inline constexpr std::pair<std::string_view, std::string_view> NextToken(std::string_view str) noexcept
		{
			const auto pos = str.find(',');
			if (pos == std::string_view::npos)
			{
				return { str, {} };
			}
			return { str.substr(0, pos), str.substr(pos + 1) };
		}

		/// @brief 再帰終端
		inline constexpr bool ParseCommaSeparatedImpl(std::string_view) noexcept
		{
			return true;
		}

		/// @brief カンマ区切りの値を順番にパースする
		template<nox::concepts::Arithmetic Head, nox::concepts::Arithmetic... Tail>
		inline constexpr bool ParseCommaSeparatedImpl(std::string_view str, Head& head, Tail&... tail) noexcept
		{
			auto [token, remaining] = NextToken(str);
			token = TrimLeft(token);

			auto v = Parse<Head>(token);
			if (!v)
			{
				return false;
			}
			head = *v;

			if constexpr (sizeof...(Tail) > 0)
			{
				return ParseCommaSeparatedImpl(TrimLeft(remaining), tail...);
			}
			return true;
		}
	}

	template<nox::concepts::Arithmetic T>
	[[nodiscard]] inline constexpr std::optional<T> Parse(std::string_view str) noexcept
	{
		T value{};
		const auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), value);
		if (ec == std::errc{})
		{
			return value;
		}
		return std::nullopt;
	}

	template<nox::concepts::Arithmetic T>
	[[nodiscard]] inline constexpr std::optional<T> Parse(std::u8string_view str) noexcept
	{
		return nox::parse::Parse<T>(std::string_view(reinterpret_cast<const char*>(str.data()), str.size()));
	}

	/// @brief "(a,b,c,...)" 形式の括弧を除去する
	/// @tparam T 
	/// @param str 
	/// @return 
	template<class T>
	[[nodiscard]] inline constexpr std::basic_string_view<T> StripParentheses(std::basic_string_view<T> str) noexcept
	{
		if (str.size() >= 2 && str.front() == '(' && str.back() == ')')
		{
			return str.substr(1, str.size() - 2);
		}
		return {};
	}

	/// @brief "(a, b, c, ...)" 形式の文字列からカンマ区切りの値をパースする
	/// @return パース成功なら true
	template<nox::concepts::Arithmetic... Values>
	[[nodiscard]] inline constexpr bool ParseCommaSeparated(std::string_view str, Values&... args) noexcept
	{
		const auto inner = nox::parse::StripParentheses(str);
		if (inner.empty())
		{
			return false;
		}

		return nox::parse::detail::ParseCommaSeparatedImpl(inner, args...);
	}

	/// @brief u8string_view 版
	template<nox::concepts::Arithmetic... Values>
	[[nodiscard]] inline constexpr bool ParseCommaSeparated(std::u8string_view str, Values&... args) noexcept
	{
		return nox::parse::ParseCommaSeparated(
			std::string_view(reinterpret_cast<const char*>(str.data()), str.size()),
			args...);
	}

	//constexpr auto ParseTest()
	//{
	//	float a, b, c;
	//	nox::parse::ParseCommaSeparated("(1, 2, 3)", a, b, c);
	//	return std::make_tuple(a, b, c);
	//}

	//inline void test()
	//{
	//	auto nnnn = ParseTest();

	//}
}