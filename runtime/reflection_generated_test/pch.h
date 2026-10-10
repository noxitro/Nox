//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	pch.h
///	@brief	reflection_generated_test のプリコンパイル済みヘッダー
///	@details	生成された entity_type_*.g.cpp は pch.h しか見ないので、購読対象のテスト用の型をここでも見せる (reflect.cpp と対になっている)。
#pragma once

#include	"../kernel/kernel.h"
#include	"../reflection/reflection.h"
#include	"../reflection/reflection_generated_register.h"
#include	"../core/core.h"

#include	"../core/test/test_types.h"
