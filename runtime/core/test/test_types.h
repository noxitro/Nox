// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

/// @file	test_types.h
/// @brief	リフレクション生成器に見せる、coreのテスト用型の集約ヘッダ。
/// @details エンジンの公開ヘッダ (core.h) にテスト用の型を載せたくないが、
///          生成器はリフレクション解析の起点 (reflection_generated/reflect.cpp) から
///          辿れる型しか見ない。そこで「生成器に見せたいテスト用型」だけをここへ集め、
///          解析の起点と reflection_generated のPCHからだけインクルードする。
///
///          このヘッダは NOX_MASTER では使わない。インクルード側で
///          #if !NOX_MASTER で括ること (ここでは括らない。括り忘れを黙って通さないため)。
#pragma once

#include	"entity_ecs_test.h"
#include	"reflection_variable_test_types.h"
