//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	pch.h
///	@brief	core_test のプリコンパイル済みヘッダー
///	@details	gtest と、依存先の代表ヘッダ (kernel.h / reflection.h / core.h / reflection_generated_test.h) を載せる。
///				リフレクションの生成物は本体 (reflection_generated) ではなく、テスト用の型を含む reflection_generated_test をリンクする。
///				test_new_delete.cpp (グローバル operator new / delete を定義する翻訳単位) は
///				PCH を使わない設定なので、ここに kernel.h を入れても new_delete.h とは同居しない。
#pragma once

#pragma warning(push, 0)
#include <gtest/gtest.h>
#pragma warning(pop)

#include	"../../kernel/kernel.h"
#include	"../../reflection/reflection.h"
#include	"../core.h"
#include	"../../reflection_generated_test/reflection_generated_test.h"
