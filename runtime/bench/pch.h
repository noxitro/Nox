//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	pch.h
///	@brief	bench_test のプリコンパイル済みヘッダー
///	@details	gtest も test_new_delete.cpp も使わないので、core/pch.h と同じくエンジンの基盤ヘッダを載せる。
///				kernel.h は <Windows.h> より先に読むこと (kernel/os/windows.h が NOMINMAX などを定義する)。
#pragma once

#include	"kernel/kernel.h"
#include	"reflection/reflection.h"
