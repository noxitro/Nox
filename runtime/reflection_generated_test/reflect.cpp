//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	reflect.cpp
///	@brief	テスト用のリフレクション解析の起点
///	@details	本体 (reflection_generated/reflect.cpp) と同じものに、テスト用の型 (core/test/test_types.h) を足して解析する。
///				core_test は本体の代わりにこのプロジェクトをリンクするので、エンジンの型もすべてここから見せる。
///				テスト用の型は全構成 (Master を含む) で見せる。runtime.exe はこのプロジェクトをリンクしない。
#include	"../kernel/kernel.h"
#include	"../reflection/reflection.h"
#include	"../reflection/reflection_generated_register.h"
#include	"../core/core.h"
#include	"../app/app.h"

#include	"../core/test/test_types.h"
