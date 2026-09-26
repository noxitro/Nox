//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	file_system.h
///	@brief	file_system
#pragma once
#include	"../nox_string.h"

namespace nox::os
{
	bool Exists(std::u8string_view path);
}

namespace nox::filesystem
{
	nox::U16String	GetCurrentPath();

	/// @brief not heap allocated
	/// @param file_path 
	/// @return 
	template<class T>
	inline constexpr std::basic_string_view<T> GetExtensions(std::basic_string_view<T> file_path, const nox::uint8 index = 0)noexcept
	{
		std::basic_string_view<T> path_view(file_path.data(), file_path.size());
		size_t last_dot_pos = path_view.rfind(T('.'));
		if (last_dot_pos == std::basic_string_view<T>::npos)
		{
			return std::basic_string_view<T>();
		}
		size_t extension_start_pos = last_dot_pos + 1;
		for (nox::uint8 i = 0; i < index; ++i)
		{
			last_dot_pos = path_view.rfind(T('.'), last_dot_pos - 1);
			if (last_dot_pos == std::basic_string_view<T>::npos)
			{
				return std::basic_string_view<T>();
			}
			extension_start_pos = last_dot_pos + 1;
		}
		return path_view.substr(extension_start_pos);
	}

	template<class T>
	class PathView
	{
	public:
		inline constexpr PathView() noexcept {}

		inline constexpr explicit PathView(std::basic_string_view<T> path) noexcept :
			path_(path)
		{
		}

		inline constexpr std::basic_string_view<T> GetStringView() const noexcept { return path_; }

		inline constexpr std::basic_string_view<T> GetExtension(const nox::uint8 index = 0) const noexcept
		{
			return nox::filesystem::GetExtensions<T>(path_, index);
		}
	private:
		std::basic_string_view<T> path_;
	};

	using U8PathView = PathView<nox::char8>;
	using U16PathView = PathView<nox::char16>;
}