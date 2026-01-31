//	Copyright (C) 2024 NOX ENGINE All Rights Rserved.

///	@file	basic_test.cpp
///	@brief	kernel の基本機能テスト

#include "stdafx.h"
#include "../basic_type.h"
#include "../string_util.h"

// 基本型のテスト
TEST(KernelBasicTest, BasicTypes)
{
	// サイズの検証
	EXPECT_EQ(sizeof(nox::s8), 1);
	EXPECT_EQ(sizeof(nox::s16), 2);
	EXPECT_EQ(sizeof(nox::s32), 4);
	EXPECT_EQ(sizeof(nox::s64), 8);
	
	EXPECT_EQ(sizeof(nox::u8), 1);
	EXPECT_EQ(sizeof(nox::u16), 2);
	EXPECT_EQ(sizeof(nox::u32), 4);
	EXPECT_EQ(sizeof(nox::u64), 8);
	
	EXPECT_EQ(sizeof(nox::f32), 4);
	EXPECT_EQ(sizeof(nox::f64), 8);
}

// StringUtil のテスト
TEST(KernelStringTest, StringLength)
{
	const char* testStr = "Hello";
	auto length = nox::StringLength(testStr);
	EXPECT_EQ(length, 5);
}

TEST(KernelStringTest, EmptyString)
{
	const char* emptyStr = "";
	auto length = nox::StringLength(emptyStr);
	EXPECT_EQ(length, 0);
}
