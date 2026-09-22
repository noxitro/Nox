//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	pch.h
///	@brief	core_test のプリコンパイル済みヘッダー
///	@details	kernel/test/pch.h と同じく gtest だけを載せる。
///				エンジン側のヘッダをここへ入れないのは、test_new_delete.cpp が
///				グローバル operator new / delete を定義する翻訳単位であり、
///				kernel の new_delete.h と同居させたくないため。
#pragma once

#pragma warning(push, 0)
#include <gtest/gtest.h>
#pragma warning(pop)
