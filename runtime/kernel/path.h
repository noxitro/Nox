//	Copyright (C) 2026 NOX ENGINE All rights reserved.

///	@file	path.h
///	@brief	path
#pragma once
//#include	<filesystem>
#include	<algorithm>
#include	<mdspan>
//#include	"advanced_type.h"
#include	"os/os_definition.h"
#include	"nox_string_view.h"

namespace nox
{
	//template<class T>
	//class BasicStringView;

	//using U8StringView = nox::BasicStringView<nox::char8>;

}

namespace nox::io
{
	/// @brief パスから複数のファイル拡張子を抽出します。
	/// @tparam T 文字列の文字型（char、wchar_tなど）。
	/// @tparam N 抽出する拡張子の最大数（デフォルト: 2）。
	/// @param path 拡張子を抽出するファイルパス。
	/// @return 拡張子の配列と見つかった拡張子の数を含むタプル。
	template<typename T, nox::uint8 N = 2>
	inline constexpr std::pair<std::array<std::basic_string_view<T>, N>, nox::uint8> GetExtensions(std::basic_string_view<T> path)noexcept
	{
		constexpr T k_dot = static_cast<T>('.');

		std::array<std::basic_string_view<T>, N> extensions{};
		nox::uint8 count = 0;
		std::size_t pos = path.size();

		while (count < N && pos > 0)
		{
			const std::size_t tmp_pos = path.rfind(k_dot, pos - 1);
			if (tmp_pos == std::basic_string_view<T>::npos)
			{
				break;
			}

			//	拡張子がドット単体（"."）の場合はスキップ
			if (tmp_pos == pos - 1)
			{
				pos = tmp_pos;
				continue;
			}

			// fix: pos - tmp_pos が正しい長さ（pos は終端インデックス）
			extensions[count++] = path.substr(tmp_pos, pos - tmp_pos);
			pos = tmp_pos;
		}

		return { extensions, count };
	}

	template<typename T, nox::uint8 N = 2>
	inline constexpr std::pair<std::array<std::basic_string_view<T>, N>, nox::uint8> GetExtensions(const T* path)noexcept
	{
		return nox::io::GetExtensions<T, N>(std::basic_string_view<T>(path));
	}
	template<typename T>
	inline constexpr std::basic_string_view<T> GetExtension(std::basic_string_view<T> path)noexcept
	{
		return nox::io::GetExtensions<T, 1>(path).first[0];
	}

	template<typename T>
	inline constexpr std::basic_string_view<T> GetExtension(const T* path)noexcept
	{
		return nox::io::GetExtension<T>(std::basic_string_view<T>(path));
	}

	class Path
	{
	public:
		inline constexpr Path() noexcept : buffer_{ 0 } {}
		inline constexpr explicit Path(std::u8string_view path) noexcept
		{
			std::size_t copy_size = std::min(path.size(), static_cast<std::size_t>(nox::os::k_max_path_length - 1));
			std::ranges::copy_n(path.data(), copy_size, buffer_.data());
			buffer_[copy_size] = u8'\0';
		}
		std::span<nox::U8StringView> GetExtensions()const noexcept;
	private:
		std::array<nox::char8, nox::os::k_max_path_length> buffer_;
	};
}