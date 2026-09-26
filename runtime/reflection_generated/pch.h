//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	stdafx.h
///	@brief	stdafx
#pragma once

#include	"../kernel/kernel.h"
#include	"../reflection/reflection.h"
#include	"../reflection/reflection_generated_register.h"
#include	"../core/core.h"

#if !NOX_MASTER
//	生成された entity_type_*.g.cpp は pch.h しか見ないので、購読対象の
//	ECSセルフテスト用の型をここでも見せる (reflect.cpp と対になっている)。
#include	"../core/test/test_types.h"
#endif // !NOX_MASTER