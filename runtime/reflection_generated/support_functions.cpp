//	Copyright (c) 2025 NOX ENGINE All rights reserved.

///	@file	support_functions.cpp
///	@brief	エラーチェック用のsandbox
#include	"stdafx.h"
#include	"support_functions.h"

#define NOX_CHECK_MEMBER(ClassType, Value)\
	NOX_VARIABLE_INFO_SETTER_MEMBER(ClassType, Value);\
	NOX_VARIABLE_INFO_GETTER_MEMBER(ClassType, Value);\
	NOX_VARIABLE_INFO_GETTER_ADDRESS_MEMBER(ClassType, Value);\
	NOX_VARIABLE_INFO_SETTER_SUBSCRIPT_MEMBER(ClassType, Value);\
	NOX_VARIABLE_INFO_GETTER_SUBSCRIPT_MEMBER(ClassType, Value);\
	NOX_VARIABLE_INFO_GETTER_ADDRESS_SUBSCRIPT_MEMBER(ClassType, Value)
// end define

#define NOX_CHECK_GLOBAL(Value)\
	NOX_VARIABLE_INFO_SETTER_GLOBAL(Value);\
	NOX_VARIABLE_INFO_GETTER_GLOBAL(Value);\
	NOX_VARIABLE_INFO_GETTER_ADDRESS_GLOBAL(Value);\
	NOX_VARIABLE_INFO_SETTER_SUBSCRIPT_GLOBAL(Value);\
	NOX_VARIABLE_INFO_GETTER_SUBSCRIPT_GLOBAL(Value);\
	NOX_VARIABLE_INFO_GETTER_ADDRESS_SUBSCRIPT_GLOBAL(Value)
// end define

//	チェック用関数
namespace nox::reflection::gen
{
	struct NonCopyable
	{
		NonCopyable() = default;
		NonCopyable(const NonCopyable&) = delete;
		NonCopyable& operator=(const NonCopyable&) = delete;
	};

	int global_value_00 = 0;
	const int global_value_01 = 0;
	constexpr int global_value_02 = 0;
	int& global_value_03 = global_value_00;
	const int& global_value_04 = global_value_00;

	int* global_value_05 = 0;
	const int* global_value_06 = 0;
	const int* const global_value_07 = 0;
	constexpr int* global_value_08 = 0;
	constexpr int* const global_value_09 = 0;
	int*& global_value_10 = global_value_05;
	const int* const& global_value_11 = global_value_05;
	NonCopyable global_value_12;

	inline void SandBoxGlobal()
	{
		NOX_CHECK_GLOBAL(global_value_00);
		NOX_CHECK_GLOBAL(global_value_01);
		NOX_CHECK_GLOBAL(global_value_02);
		NOX_CHECK_GLOBAL(global_value_03);
		NOX_CHECK_GLOBAL(global_value_04);
		NOX_CHECK_GLOBAL(global_value_05);
		NOX_CHECK_GLOBAL(global_value_06);
		NOX_CHECK_GLOBAL(global_value_07);
		NOX_CHECK_GLOBAL(global_value_08);
		NOX_CHECK_GLOBAL(global_value_09);
		NOX_CHECK_GLOBAL(global_value_10);
		NOX_CHECK_GLOBAL(global_value_11);
		NOX_CHECK_GLOBAL(global_value_12);
	}

	struct Class
	{
		int value_00 = 0;
		const int value_01 = 0;
		static constexpr int value_02 = 0;
		int& value_03 = value_00;
		const int& value_04 = value_00;

		int* value_05 = 0;
		const int* value_06 = 0;
		const int* const value_07 = 0;
		static constexpr int* value_08 = 0;
		static constexpr int* const value_09 = 0;
		int*& value_10 = value_05;
		const int* const& value_11 = value_05;
		NonCopyable value_12;
	};

	inline void SandBoxMember()
	{
		NOX_CHECK_MEMBER(Class, Class::value_00);
		NOX_CHECK_MEMBER(Class, Class::value_01);
		NOX_CHECK_MEMBER(Class, Class::value_02);
		NOX_CHECK_MEMBER(Class, Class::value_03);
		NOX_CHECK_MEMBER(Class, Class::value_04);
		NOX_CHECK_MEMBER(Class, Class::value_05);
		NOX_CHECK_MEMBER(Class, Class::value_06);
		NOX_CHECK_MEMBER(Class, Class::value_07);
		NOX_CHECK_MEMBER(Class, Class::value_08);
		NOX_CHECK_MEMBER(Class, Class::value_09);
		NOX_CHECK_MEMBER(Class, Class::value_10);
		NOX_CHECK_MEMBER(Class, Class::value_11);
		NOX_CHECK_MEMBER(Class, Class::value_12);

	}
}