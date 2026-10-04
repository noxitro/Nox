//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	allocation_counter.h
///	@brief	nox::memory::Allocate / Deallocate の累積カウンタ
///	@details	「1 操作あたり何回ヒープ確保したか」をベンチマーク (runtime/bench) で数えるためのもの。
///				確保回数は実行時間と違ってマシンの負荷で揺れないので、CI で比較・判定できる。
///
///				数えられる経路: グローバル operator new (memory/new_delete.h)、nox::Object::operator new、
///				StlAllocateAdapter (nox::Vector など)、PmrMemoryResource、Archetype の Chunk 確保など、
///				nox::memory::Allocate を通るものすべて。std::malloc の直呼びや DLL 内の確保は数えない。
///
///				カウンタは Allocate / Deallocate が元々取っているヒープ一覧のロックの内側で加算する。
///				新しい同期を足さないのでコストは加算 2 回ぶんだけで、全スレッドの合計が正確に取れる。
///				そのため Master を含む全構成で有効にしてある。
///
///				ReflectionGenerator の解析対象に入れないため、kernel.h は NOX_REFLECTION_GENERATOR の間だけ include を外す。
#pragma once
#include	"../basic_type.h"

namespace nox::memory
{
	/// @brief プロセス開始からの確保・解放の累積値 (全スレッド合計)
	struct AllocationCounters final
	{
		/// @brief Allocate が呼ばれた回数
		nox::uint64 allocate_count = 0u;

		/// @brief Allocate へ要求されたバイト数の累計 (管理ヘッダや 16 バイト丸めは含まない)
		nox::uint64 allocate_bytes = 0u;

		/// @brief Deallocate が呼ばれた回数
		nox::uint64 deallocate_count = 0u;
	};

	/// @brief		現在の累積値を取得する
	/// @details	ヒープ一覧のロックを取って読むので、他スレッドの確保と食い違わない。
	///				計測区間の前後で 2 回呼び、差を取って使う。
	[[nodiscard]] nox::memory::AllocationCounters GetAllocationCounters()noexcept;
}
