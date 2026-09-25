// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

/// @file	asset.cpp
/// @brief	asset
#include "pch.h"

#include "asset.h"
#include "asset_format.h"

namespace nox
{
	namespace
	{
		//	オンディスクのチャンクヘッダはパディングなし（type:u8 + offset:u32 + size:u32 + version:u32）
		constexpr std::size_t k_chunk_header_disk_size = sizeof(nox::uint8) + sizeof(nox::uint32) * 3;

		/// @brief バイト列の現在位置からリトルエンディアンのPODを読み出す
		template<class T> requires(std::is_trivially_copyable_v<T>)
		bool ReadLe(std::span<const std::byte> bytes, std::size_t& cursor, T& out) noexcept
		{
			if (cursor + sizeof(T) > bytes.size())
			{
				return false;
			}
			std::memcpy(&out, bytes.data() + cursor, sizeof(T));
			cursor += sizeof(T);
			return true;
		}

		/// @brief Main チャンクの範囲を特定する
		/// @return 見つかった場合 true。offset/size を out に設定
		bool FindMainChunk(std::span<const std::byte> bytes, nox::uint16 chunk_count, nox::uint32& out_offset, nox::uint32& out_size) noexcept
		{
			std::size_t cursor = sizeof(nox::AssetFileHeader);	//	ヘッダ(8byte)はパディングなしで sizeof と一致
			for (nox::uint16 i = 0; i < chunk_count; ++i)
			{
				nox::uint8 type = 0;
				nox::uint32 offset = 0;
				nox::uint32 size = 0;
				nox::uint32 version = 0;
				if (ReadLe(bytes, cursor, type) == false ||
					ReadLe(bytes, cursor, offset) == false ||
					ReadLe(bytes, cursor, size) == false ||
					ReadLe(bytes, cursor, version) == false)
				{
					return false;
				}

				if (static_cast<nox::AssetChunkType>(type) == nox::AssetChunkType::Main)
				{
					out_offset = offset;
					out_size = size;
					return true;
				}
			}
			return false;
		}
	}
}

void nox::Asset::Bind(std::u8string_view path, nox::AssetManager& manager)
{
	manager_ = manager;
	path_ = nox::U8String(path);
}

bool nox::Asset::Initialize(std::u8string_view native_path)
{
	is_initialized_ = false;

	//	ネイティブファイルをメモリマップする（ペイロードはゼロコピーで in-place 参照）
	nox::os::ReadOnlyMappedFile mapped;
	if (mapped.Open(native_path) == false)
	{
		return false;
	}

	const std::span<const std::byte> bytes = mapped.GetView();

	//	ファイルヘッダを読み出し、マジック/バージョンを検証する（ネイティブコンバート完了の確認）
	std::size_t cursor = 0;
	nox::uint32 magic = 0;
	nox::uint16 version = 0;
	nox::uint16 chunk_count = 0;
	if (ReadLe(bytes, cursor, magic) == false ||
		ReadLe(bytes, cursor, version) == false ||
		ReadLe(bytes, cursor, chunk_count) == false)
	{
		return false;
	}
	if (magic != nox::kAssetMagic || version != nox::kAssetFormatVersion)
	{
		return false;
	}

	//	Main チャンクを特定する
	nox::uint32 main_offset = 0;
	nox::uint32 main_size = 0;
	if (FindMainChunk(bytes, chunk_count, main_offset, main_size) == false)
	{
		return false;
	}
	if (static_cast<std::size_t>(main_offset) + static_cast<std::size_t>(main_size) > bytes.size())
	{
		return false;
	}

	// AssetChunkLoaderを作成
	nox::AssetChunkLoader loader;

	//	Main チャンクのビュー上に（ヒープを使わない）ストリームリーダーを構築して初期化する。
	//	SpanStreamReader は読み取り専用（buffer_ から copy するのみ）のため、const を外しても安全。
	const std::span<const std::byte> main_view = bytes.subspan(main_offset, main_size);
	std::span<std::byte> main_mutable{ const_cast<std::byte*>(main_view.data()), main_view.size() };
	nox::io::SpanStreamReader span_reader(main_mutable);
	nox::io::BinaryReader binary_reader(span_reader);

	is_initialized_ = OnInitialize(binary_reader);
	return is_initialized_;
}
