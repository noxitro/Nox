#pragma once
#include	"../basic_type.h"

namespace nox::memory
{
	/// @brief メモリ確保のインスタンスタイプ
	enum class InstanceType : nox::uint8
	{
		/// @brief runtimeのObject継承クラス
		Object,

		/// @brief rutnimeのManagedObject継承クラス
		ManagedObject,

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
}