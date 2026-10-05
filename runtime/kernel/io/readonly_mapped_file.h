//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	readonly_mapped_file.h
///	@brief	readonly_mapped_file
#pragma once
#include	<string_view>
#include	<cstddef>
#include	<span>

namespace nox::io
{
	/// @brief 読み取り専用メモリマップトファイル
	/// @details ファイル全体をマップし、ヒープコピーなしで in-place 参照するための型。
	class ReadOnlyMappedFile final
	{
	public:
		inline constexpr ReadOnlyMappedFile() noexcept :
			file_handle_(nullptr),
			mapping_handle_(nullptr),
			data_(nullptr),
			size_(0)
		{
		}

		~ReadOnlyMappedFile() noexcept;

		ReadOnlyMappedFile(const ReadOnlyMappedFile&) = delete;
		ReadOnlyMappedFile& operator=(const ReadOnlyMappedFile&) = delete;

		/// @brief ファイルをマップする
		/// @param path ネイティブパス
		/// @return 成功したか（オープン不可・サイズ0・マップ失敗は false）
		bool Open(std::u8string_view path);
		void Close() noexcept;

		inline constexpr bool IsOpen()const noexcept { return data_ != nullptr; }

		/// @brief マップ領域全体のビュー
		inline std::span<const std::byte> GetView()const noexcept { return { data_, size_ }; }
		inline constexpr std::size_t GetSize()const noexcept { return size_; }

	private:
		void* file_handle_;
		void* mapping_handle_;
		const std::byte* data_;
		std::size_t size_;
	};
}