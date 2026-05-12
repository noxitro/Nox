//	Copyright (C) 2024 NOX ENGINE All Rights Rserved.

///	@file	basic_test.cpp
///	@brief	kernel の基本機能テスト

#include "pch.h"
#include "../basic_type.h"
#include "../reflection_type_definition.h"
#include "../reflection_type_utility.h"
#include "../type_traits/function_signature.h"

namespace
{
	struct ReflectionTypeUtilityTestType
	{
		int value{};
		static inline int static_value = 0;

		void MemberFunction() noexcept {}
		static void StaticFunction() noexcept {}
	};

	void FreeFunction() noexcept {}
	int g_free_value = 0;
}

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

TEST(KernelBasicTest, MemberFunctionsAreNotMarkedStatic)
{
	using MemberFunction = nox::ToMemberFunctionPointerType<void() noexcept, ReflectionTypeUtilityTestType>;
	using StaticFunction = decltype(&ReflectionTypeUtilityTestType::StaticFunction);
	using FreeFunctionType = decltype(&FreeFunction);

	EXPECT_FALSE(nox::util::IsBitAnd(
		nox::reflection::GetFunctionAttributeFlags<MemberFunction>(),
		nox::reflection::FunctionAttributeFlag::Static));
	EXPECT_TRUE(nox::util::IsBitAnd(
		nox::reflection::GetFunctionAttributeFlags<StaticFunction>(),
		nox::reflection::FunctionAttributeFlag::Static));
	EXPECT_TRUE(nox::util::IsBitAnd(
		nox::reflection::GetFunctionAttributeFlags<FreeFunctionType>(),
		nox::reflection::FunctionAttributeFlag::Static));
}

TEST(KernelBasicTest, MemberFieldsAreNotMarkedStatic)
{
	using MemberField = decltype(&ReflectionTypeUtilityTestType::value);
	using StaticField = decltype(&ReflectionTypeUtilityTestType::static_value);
	using FreeField = decltype(&g_free_value);

	EXPECT_FALSE(nox::util::IsBitAnd(
		nox::reflection::GetFieldAttributeFlags<MemberField>(),
		nox::reflection::VariableAttributeFlag::Static));
	EXPECT_TRUE(nox::util::IsBitAnd(
		nox::reflection::GetFieldAttributeFlags<StaticField>(),
		nox::reflection::VariableAttributeFlag::Static));
	EXPECT_TRUE(nox::util::IsBitAnd(
		nox::reflection::GetFieldAttributeFlags<FreeField>(),
		nox::reflection::VariableAttributeFlag::Static));
}

