//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	core_self_test.cpp
///	@brief	NOX_ASSERT ベースのセルフテストを GoogleTest から走らせる。
///	@details	これらの本体はかつて core/test_support/ にあり、core.vcxproj が
///				core 本体へ混ぜてビルドしていた。呼び出し元は runtime.exe の
///				nox::EntryPoint だけで (しかも TestEntityEcs / TestJobSystem の 2 本だけ)、
///				CI のテストゲートには載っていなかった。
///				起動時テストは廃止し、本体をこのディレクトリへ移して
///				ここから全部呼び、gtest のケースとして数えている。
///
///	@note		中身は NOX_ASSERT のままなので、失敗しても gtest の
///				EXPECT 失敗としてではなくアサート (Debug では __debugbreak) で止まる。
///				本来は gtest のマクロへ書き換えるべきだが、
///				それは各テストの中身に手を入れることになるので分けてある。

#include	"pch.h"

#include	"test.h"
#include	"test_reflection.h"

TEST(CoreSelfTest, Reflection)
{
	nox::test::TestReflection();
}

TEST(CoreSelfTest, Delegate)
{
	nox::test::TestDelegate();
}

TEST(CoreSelfTest, EntityCommandBuffer)
{
	nox::test::TestEntityCommandBuffer();
}

TEST(CoreSelfTest, EntityEcs)
{
	nox::test::TestEntityEcs();
}

TEST(CoreSelfTest, JobSystem)
{
	nox::test::TestJobSystem();
}
