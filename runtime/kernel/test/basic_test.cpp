//	Copyright (C) 2024 NOX ENGINE All Rights Rserved.

///	@file	basic_test.cpp
///	@brief	kernel の基本機能テスト

#include "stdafx.h"
#include "../basic_type.h"

// 基本型のテスト
TEST(KernelBasicTest, BasicTypes)
{
	// サイズの検証
	EXPECT_EQ(sizeof(nox::int8), 1);
	EXPECT_EQ(sizeof(nox::int16), 2);
	EXPECT_EQ(sizeof(nox::int32), 4);
	EXPECT_EQ(sizeof(nox::int64), 8);
	
	EXPECT_EQ(sizeof(nox::uint8), 1);
	EXPECT_EQ(sizeof(nox::uint16), 2);
	EXPECT_EQ(sizeof(nox::uint32), 4);
	EXPECT_EQ(sizeof(nox::uint64), 8);
	
	EXPECT_EQ(sizeof(nox::float_t), 4);
	EXPECT_EQ(sizeof(nox::double_t), 8);
}

// ポインタ型のテスト
TEST(KernelBasicTest, PointerTypes)
{
	// intptr と uintptr のサイズはプラットフォーム依存
	// x64 では 8 バイトのはず
	EXPECT_EQ(sizeof(nox::intptr), sizeof(void*));
	EXPECT_EQ(sizeof(nox::uintptr), sizeof(void*));
}

