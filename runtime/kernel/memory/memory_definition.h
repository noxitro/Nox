// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

#pragma once
#include	<bit>
#include	"../basic_type.h"

namespace nox::memory
{
	/// @brief メモリ確保のインスタンスタイプ
	enum class InstanceType : nox::uint8
	{
		/// @brief runtimeのObject継承クラス
		Object,

		/// @brief stlから確保したメモリ
		Stl,

		/// @brief その他 通常クラス
		Other,

		_Max,
	};

	/// @brief メモリセグメントタイプ
	enum class SegmentType : nox::uint8
	{
		/// @brief		デフォルト
		/// @details	指定がない場合はこのセグメントに属する
		Default,

		/// @brief		メモリ管理の初期化処理が行われるまでのセグメント
		Boot,

		/// @brief		外部リソース
		Resource,

		/// @brief		描画
		Render,

		/// @brief		開発用セグメント
		Develop,

		_Max
	};

	/// @brief ヒープ情報
	///@details	ヒープ情報は32byte以下であることを保証
	struct alignas(16) HeapInfo
	{
		/// @brief 次のヒープ情報
		HeapInfo* next;

		/// @brief 前のヒープ情報
		HeapInfo* prev;

		/// @brief サイズ
		nox::uint32 size;

		/// @brief アライメントサイズ
		nox::uint32 align_size;

		/// @brief メモリプロファイラ用ハンドル
		nox::uint16 profile_handle;

		/// @brief ヒープ情報の整合性チェック用マジックナンバー
		nox::uint8 magic;

		/// @brief インスタンスタイプ
		nox::memory::InstanceType instance_type : 4;

		/// @brief セグメントタイプ
		nox::memory::SegmentType segment_type : 4;
	};

	/// @brief 移行予定のヒープ情報(16byte)
	struct HeapInfo2
	{
		/// @brief Header、Padding、payloadを含めた実際の割り当てブロック数
		/// @details	Header：16byte HeapInfo
		///				Padding：要求アライメントにそろえるためにスキップする領域
		///				payload：ユーザーが要求した領域
		nox::uint32 use_block;

		/// @brief 割り当て後、Arena末尾までのブロック数
		nox::uint32 spare_block;
	
		/// @brief 次に処理すべきブロックのインデックス GC用(未実装)
		nox::uint16 next_zct_block;

		/// @brief		要求アラインに揃えるために先頭でスキップしたブロック数
		/// @details	Header位置からpayload先頭までの距離をブロック単位で復元
		nox::uint16 align_block;

		/// @brief メモリプロファイラ用ハンドル
		nox::uint16 profile_handle;

		/// @brief アライメント値のlog2値
		/// @details 例えば、16byteアライメント要求ならば4(2^4)
		nox::uint8 align_bits:4;

		/// @brief ブロック切り上げで余ったバイト数(0~15)
		nox::uint8 diff_size : 4;

		/// @brief インスタンスタイプ
		nox::memory::InstanceType instance_type : std::bit_width(static_cast<nox::uint8>(nox::memory::InstanceType::_Max));

		/// @brief セグメント
		nox::memory::SegmentType segment_type : std::bit_width(static_cast<nox::uint8>(nox::memory::SegmentType::_Max));

		// 予約領域
		bool reserved:1;
	};
	static_assert(sizeof(HeapInfo2) == 16, "HeapInfo size is not 16 bytes");

	/// @brief		Arena
	/// @details	Header:32byte, body:64KB-32byte
	struct Arena
	{
		/// @brief 構造体のサイズ
		static constexpr nox::uint32 size = 1024 * 64;
		static constexpr nox::uint32 mask = size - 1;

		/// @brief ヘッダサイズ
		static constexpr nox::uint32 head = 32;

		/// @brief 次のアリーナ
		nox::memory::Arena* next_arena;

		/// @brief 空のヒープ情報を指すポインタ
		nox::memory::HeapInfo2* empty_heap_info;

		nox::uint32 alloc_block;
		nox::uint32 free_block;
		nox::uint32 next_zct_block;

		/// @brief 実際に
	//	nox::uint8 body[size - head];
	};

	//static_assert(sizeof(Arena) - sizeof(Arena::body) == 32);
	//static_assert(sizeof(Arena) == nox::memory::Arena::size);
}