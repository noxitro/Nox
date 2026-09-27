//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	FileWin64.h
///	@brief	FileWin64
#pragma once
#include	"../../basic_definition.h"
#include	"file_base.h"

#if NOX_WIN64
namespace nox::os::detail
{
	class FileWin64 final : public nox::os::FileBase
	{
	public:
		inline constexpr FileWin64() noexcept :
			native_file_handle_(nullptr)
		{}

		~FileWin64() noexcept;

		bool Open(std::u8string_view path, std::u8string_view mode);
		void Close() noexcept;

		inline constexpr bool IsOpen()const noexcept { return native_file_handle_ != nullptr; }
		
		void Write(std::span<const std::byte> src)const;
		std::span<std::byte> Read(std::span<std::byte> dest)const;

		/// @brief ファイルサイズ（バイト数）を取得する
		/// @return ファイルサイズ。オープンされていない、または取得に失敗した場合は0
		nox::uint64 GetSize()const;

		inline constexpr void* GetNativeHandle()const noexcept { return native_file_handle_; }

	private:
		void* native_file_handle_;
	};

	/// @brief 読み取り専用メモリマップトファイル
	/// @details ファイル全体をマップし、ヒープコピーなしで in-place 参照するための型。
	class ReadOnlyMappedFileWin64 final
	{
	public:
		inline constexpr ReadOnlyMappedFileWin64() noexcept :
			file_handle_(nullptr),
			mapping_handle_(nullptr),
			data_(nullptr),
			size_(0)
		{}

		~ReadOnlyMappedFileWin64() noexcept;

		ReadOnlyMappedFileWin64(const ReadOnlyMappedFileWin64&) = delete;
		ReadOnlyMappedFileWin64& operator=(const ReadOnlyMappedFileWin64&) = delete;

		/// @brief ファイルをマップする
		/// @param path ネイティブパス
		/// @return 成功したか（オープン不可・サイズ0・マップ失敗は false）
		bool Open(std::u8string_view path);
		void Close() noexcept;

		inline constexpr bool IsOpen()const noexcept { return data_ != nullptr; }

		/// @brief マップ領域全体のビュー
		inline std::span<const std::byte> GetView()const noexcept { return { data_, size_ }; }
		inline constexpr nox::uint64 GetSize()const noexcept { return size_; }

	private:
		void* file_handle_;
		void* mapping_handle_;
		const std::byte* data_;
		std::size_t size_;
	};
}
#endif