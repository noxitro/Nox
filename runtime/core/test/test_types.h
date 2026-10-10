// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

/// @file	test_types.h
/// @brief	リフレクション生成器に見せる、coreのテスト用型の集約ヘッダ。
/// @details エンジンの公開ヘッダ (core.h) にテスト用の型を載せたくないが、
///          生成器はリフレクション解析の起点 (reflect.cpp) から辿れる型しか見ない。
///          そこで「生成器に見せたいテスト用型」だけをここへ集め、テスト用の生成プロジェクト
///          reflection_generated_test の解析の起点と PCH からだけインクルードする (全構成)。
///
///          本体の reflection_generated (runtime.exe がリンクする) からはインクルードしない。
///          ここの EntitySystem / EntityLogic が runtime.exe に購読され、毎フレームの更新に乗るため。
#pragma once

#include	"entity_ecs_test.h"
#include	"reflection_variable_test_types.h"
