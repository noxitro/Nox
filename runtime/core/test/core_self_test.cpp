//	Copyright (C) 2026 NOX ENGINE All rights reserved.

///	@file	core_self_test.cpp
///	@brief	core/test_support/ にあった NOX_ASSERT ベースのセルフテストを GoogleTest から走らせる。
///	@details	core/test_support/*.cpp は core.vcxproj でビルドされてはいるものの、
///				gtest ではなく NOX_ASSERT で書かれており、
///				呼び出し元は runtime.exe の nox::EntryPoint だけだった
///				(しかも TestEntityEcs / TestJobSystem の 2 本しか呼ばれていない)。
///				CI のテストゲートには一切載っていなかったので、
///				ここから全部呼んで gtest のケースとして数える。
///
///	@note		中身は NOX_ASSERT のままなので、失敗しても gtest の
///				EXPECT 失敗としてではなくアサート (Debug では __debugbreak) で止まる。
///				本来は gtest のマクロへ書き換えるべきだが、
///				それは各テストの中身に手を入れることになるので分けてある。

#include	"pch.h"

#include	"../test_support/test.h"
#include	"../test_support/test_reflection.h"

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
