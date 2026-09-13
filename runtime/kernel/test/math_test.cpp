//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	math_test.cpp
///	@brief	kernel の数学ライブラリテスト

#include "pch.h"
#include "../math/vector2d.h"
#include "../math/vector3d.h"

// Vector2D のテスト
TEST(KernelMathTest, Vec2Construction)
{
	nox::Vec2 vec(1.0f, 2.0f);
	EXPECT_FLOAT_EQ(vec.x, 1.0f);
	EXPECT_FLOAT_EQ(vec.y, 2.0f);
}

TEST(KernelMathTest, Vec2DefaultConstruction)
{
	nox::Vec2 vec;
	EXPECT_FLOAT_EQ(vec.x, 0.0f);
	EXPECT_FLOAT_EQ(vec.y, 0.0f);
}

TEST(KernelMathTest, Vec2Addition)
{
	nox::Vec2 vec1(1.0f, 2.0f);
	nox::Vec2 vec2(3.0f, 4.0f);
	nox::Vec2 result = vec1 + vec2;
	EXPECT_FLOAT_EQ(result.x, 4.0f);
	EXPECT_FLOAT_EQ(result.y, 6.0f);
}

TEST(KernelMathTest, Vec2Subtraction)
{
	nox::Vec2 vec1(5.0f, 7.0f);
	nox::Vec2 vec2(2.0f, 3.0f);
	nox::Vec2 result = vec1 - vec2;
	EXPECT_FLOAT_EQ(result.x, 3.0f);
	EXPECT_FLOAT_EQ(result.y, 4.0f);
}

TEST(KernelMathTest, Vec2ScalarMultiplication)
{
	nox::Vec2 vec(2.0f, 3.0f);
	nox::Vec2 result = vec * 2.0f;
	EXPECT_FLOAT_EQ(result.x, 4.0f);
	EXPECT_FLOAT_EQ(result.y, 6.0f);
}

// Vector3D のテスト
TEST(KernelMathTest, Vec3Construction)
{
	nox::Vec3 vec(1.0f, 2.0f, 3.0f);
	EXPECT_FLOAT_EQ(vec.x, 1.0f);
	EXPECT_FLOAT_EQ(vec.y, 2.0f);
	EXPECT_FLOAT_EQ(vec.z, 3.0f);
}

TEST(KernelMathTest, Vec3DefaultConstruction)
{
	nox::Vec3 vec;
	EXPECT_FLOAT_EQ(vec.x, 0.0f);
	EXPECT_FLOAT_EQ(vec.y, 0.0f);
	EXPECT_FLOAT_EQ(vec.z, 0.0f);
}

