// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

/// @file	asset_format.h
/// @brief	ネイティブアセットファイルのオンディスクフォーマット定義
#pragma once
#include	"../kernel/basic_type.h"

//	オンディスクレイアウト（リトルエンディアン）:
//	[AssetFileHeader]
//	[AssetChunkHeader] * chunk_count
//	[RawData...]
//	- offset / size はファイル先頭からの絶対バイト位置・バイト数。
//	- Main チャンク（AssetChunkType::Main）が必須。
//	- フィールドは明示シリアライズ（C/C# の構造体パディングに依存しない）。
namespace nox
{
	/// @brief チャンク種別
	enum class AssetChunkType : nox::uint8
	{
		Main,
		Extra,
	};

	/// @brief ネイティブアセットのマジック（'N''O''X''A' をリトルエンディアン u32 で表現）
	inline constexpr nox::uint32 kAssetMagic = 0x41584F4Eu;

	/// @brief 現在のフォーマットバージョン
	inline constexpr nox::uint16 kAssetFormatVersion = 1;

	/// @brief ファイル先頭ヘッダ
	struct AssetFileHeader
	{
		nox::uint32 magic;			///< kAssetMagic
		nox::uint16 version;		///< kAssetFormatVersion
		nox::uint16 chunk_count;	///< 後続の AssetChunkHeader 個数
	};

	/// @brief チャンクヘッダ
	struct AssetChunkHeader
	{
		nox::AssetChunkType type;	///< チャンク種別
		nox::uint32 offset;			///< ファイル先頭からのデータオフセット
		nox::uint32 size;			///< データサイズ（バイト）
		nox::uint32 version;		///< チャンクバージョン
	};
}
