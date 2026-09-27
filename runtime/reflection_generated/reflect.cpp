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

#if !NOX_MASTER
//	ECSセルフテスト用の型。ヘッダに定義するだけで購読されることの実証を兼ねる。
//	エンジンの公開ヘッダには載せたくないので、解析の起点であるここから直接見せる。
#include	"../core/test/test_types.h"
#endif // !NOX_MASTER