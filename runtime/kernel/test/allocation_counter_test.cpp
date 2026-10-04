//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	allocation_counter_test.cpp
///	@brief	nox::memory::GetAllocationCounters の検証
///	@details	ベンチマーク (runtime/bench) は「1 操作あたりの確保回数」をこのカウンタで数え、
///				予算を超えたら CI を落とす。数え漏れや二重計上があると判定そのものが狂うので、
///				Allocate / Deallocate の直呼びと、StlAllocateAdapter 経由の確保が
///				ちょうど 1 回ずつ数えられることを確かめる。
///
///	@note		kernel_test のグローバル operator new は test_new_delete.cpp で CRT へ差し替えてあり、
///				gtest 側の確保はこのカウンタに乗らない。ここで見えるのは nox::memory を通った確保だけ。

#include "pch.h"
#include "../basic_type.h"
#include "../advanced_type.h"
#include "../memory/nox_memory.h"
#include "../memory/allocation_counter.h"
#include "../vector.h"

///	@brief	Allocate / Deallocate 1 回ずつが、要求サイズとともにそのまま数えられる。
TEST(AllocationCounter, CountsAllocateAndDeallocate)
{
	const nox::memory::AllocationCounters before = nox::memory::GetAllocationCounters();

	void* const pointer = nox::memory::Allocate(40u, nox::memory::InstanceType::Other);
	ASSERT_NE(pointer, nullptr);

	const nox::memory::AllocationCounters allocated = nox::memory::GetAllocationCounters();
	EXPECT_EQ(allocated.allocate_count - before.allocate_count, 1u);
	EXPECT_EQ(allocated.allocate_bytes - before.allocate_bytes, 40u);
	EXPECT_EQ(allocated.deallocate_count - before.deallocate_count, 0u);

	nox::memory::Deallocate(pointer);

	const nox::memory::AllocationCounters released = nox::memory::GetAllocationCounters();
	EXPECT_EQ(released.allocate_count - allocated.allocate_count, 0u);
	EXPECT_EQ(released.deallocate_count - allocated.deallocate_count, 1u);
}

///	@brief	nox::Vector の reserve は StlAllocateAdapter 経由で 1 回だけ数えられる。
///	@details	Debug では MSVC STL のコンテナプロキシも同じアロケータから取られるが、
///				それはコンストラクタで済んでいるので、reserve の前後だけを比べれば構成によらず 1 回になる。
TEST(AllocationCounter, CountsStlAllocateAdapter)
{
	nox::Vector<nox::int32> values;

	const nox::memory::AllocationCounters before = nox::memory::GetAllocationCounters();
	values.reserve(8u);
	const nox::memory::AllocationCounters after = nox::memory::GetAllocationCounters();

	EXPECT_EQ(after.allocate_count - before.allocate_count, 1u);
	EXPECT_EQ(after.allocate_bytes - before.allocate_bytes, sizeof(nox::int32) * 8u);
	EXPECT_EQ(after.deallocate_count - before.deallocate_count, 0u);
}

///	@brief	確保を伴わない操作ではカウンタが動かない (予算 0 のベンチが誤判定しないこと)。
TEST(AllocationCounter, DoesNotMoveWithoutAllocation)
{
	nox::Vector<nox::int32> values;
	values.reserve(16u);

	const nox::memory::AllocationCounters before = nox::memory::GetAllocationCounters();
	for (nox::int32 index = 0; index < 16; ++index)
	{
		values.push_back(index);
	}
	values.clear();
	const nox::memory::AllocationCounters after = nox::memory::GetAllocationCounters();

	EXPECT_EQ(after.allocate_count, before.allocate_count);
	EXPECT_EQ(after.allocate_bytes, before.allocate_bytes);
	EXPECT_EQ(after.deallocate_count, before.deallocate_count);
}
