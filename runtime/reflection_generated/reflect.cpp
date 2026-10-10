//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	reflect.cpp
///	@brief	リフレクション解析対象のファイル
//#include	"pch.h"
#include	"reflect.h"

#include	"../kernel/kernel.h"
#include	"../reflection/reflection.h"
#include	"../reflection/reflection_generated_register.h"
#include	"../core/core.h"
#include	"../app/app.h"

//	テスト用の型 (core/test/test_types.h) はここに足さない。
//	ここから見えた型は runtime.exe に購読され、毎フレームの更新に乗る。
//	テスト用の型は reflection_generated_test (core_test だけがリンクする) で生成する。