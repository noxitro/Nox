//	Copyright (C) 2024 NOX ENGINE All Rights Rserved.

///	@file	basic_test.cpp
///	@brief	kernel の基本機能テスト

#include "pch.h"
#include "../basic_type.h"
#include "../reflection_type_definition.h"
#include "../reflection_type_utility.h"
#include "../type_traits/function_signature.h"
#include "../type_traits/type_name.h"

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

//	型名の正規化を確かめるための型。綴りを期待値に固定したいので、
//	ツールセットで綴りが割れる無名名前空間には置かない。
struct TypeNameProbe
{
	int value{};
};

namespace type_name_probe
{
	struct Nested
	{
		struct Value
		{
			int value{};
		};

		enum class Kind : nox::uint8
		{
			A,
		};
	};

	template<class T>
	struct Box
	{
		T value{};
	};

	template<class T, class U>
	struct Pair
	{
		T first{};
		U second{};
	};
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

//	nox::util::GetTypeName は MSVC / clang-cl で同じ綴りになるよう正規化してある。
//	期待値をリテラルで書けること自体が、ツールセット間で一致している証拠になる
//	(修正前は MSVC が "struct TypeNameProbe"、clang-cl が " TypeNameProbe" を返していた)。
TEST(KernelBasicTest, TypeNameIsNormalizedAcrossToolsets)
{
	EXPECT_EQ(nox::util::GetTypeName<TypeNameProbe>(), "TypeNameProbe");
	EXPECT_EQ(nox::util::GetTypeName<type_name_probe::Nested::Value>(), "type_name_probe::Nested::Value");
	EXPECT_EQ(nox::util::GetTypeName<type_name_probe::Nested::Kind>(), "type_name_probe::Nested::Kind");
	EXPECT_EQ(nox::util::GetTypeName<type_name_probe::Box<TypeNameProbe>>(), "type_name_probe::Box<TypeNameProbe>");
	EXPECT_EQ(
		(nox::util::GetTypeName<type_name_probe::Pair<nox::int32, type_name_probe::Nested::Kind>>()),
		"type_name_probe::Pair<int,type_name_probe::Nested::Kind>");
	EXPECT_EQ(nox::util::GetTypeName<TypeNameProbe*>(), "TypeNameProbe*");
	EXPECT_EQ(nox::util::GetTypeName<const TypeNameProbe&>(), "const TypeNameProbe&");
	EXPECT_EQ(nox::util::GetTypeName<const TypeNameProbe* const*>(), "const TypeNameProbe*const*");
	EXPECT_EQ(nox::util::GetTypeName<nox::int64>(), "long long");
	EXPECT_EQ(nox::util::GetTypeName<nox::uint64>(), "unsigned long long");
	EXPECT_EQ(nox::util::GetTypeName<void>(), "void");
}

//	型IDは正規化済みの型名のCRC32。ツールセットによらず同じ値になる。
TEST(KernelBasicTest, UniqueTypeIdIsCrc32OfTypeName)
{
	EXPECT_EQ(nox::util::GetUniqueTypeID<TypeNameProbe>(), nox::util::Crc32(nox::util::GetTypeName<TypeNameProbe>()));
	EXPECT_EQ(nox::util::GetUniqueTypeID<nox::int64>(), nox::util::Crc32(nox::util::GetTypeName<nox::int64>()));
	EXPECT_NE(nox::util::GetUniqueTypeID<TypeNameProbe>(), nox::util::GetUniqueTypeID<type_name_probe::Nested::Value>());
}

