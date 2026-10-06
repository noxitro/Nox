//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	file.h
///	@brief	file
#pragma once
#include	<string_view>
#include	<span>
#include	"../basic_definition.h"

namespace nox::io
{
	class File final
	{
	public:
		inline constexpr File() noexcept :
			native_file_handle_(nullptr)
		{
		}

		~File() noexcept;

		bool Open(std::u8string_view path, std::u8string_view mode);
		void Close() noexcept;

		inline constexpr bool IsOpen()const noexcept { return native_file_handle_ != nullptr; }

		void Write(std::span<const std::byte> src)const;
		std::span<std::byte> Read(std::span<std::byte> dest)const;

		/// @brief ファイルサイズ（バイト数）を取得する
		/// @return ファイルサイズ。オープンされていない、または取得に失敗した場合は0
		std::size_t GetSize()const;

		inline constexpr void* GetNativeHandle()const noexcept { return native_file_handle_; }

	private:
		void* native_file_handle_;
	};
}
