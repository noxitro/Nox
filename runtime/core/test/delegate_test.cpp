//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	delegate_test.cpp
///	@brief	delegate_test
#include	"pch.h"

#include	"../../kernel/kernel.h"
#include	"../../reflection/reflection.h"
#include	"test.h"

#include	"../../kernel/assertion.h"
#include	"../../kernel/delegate.h"

static_assert(sizeof(nox::MoveOnlyDelegate<void()>) == 40);
static_assert(sizeof(nox::CopyableDelegate<void()>) == 40);

void nox::test::TestDelegate()
{
	int32 invocation_count = 0;
	MoveOnlyDelegate<void(int)> delegate{ [&invocation_count](int value) noexcept { invocation_count += value; } };
	IDelegate<void(int)>& delegate_view = delegate;
	delegate_view(4);
	NOX_ASSERT(invocation_count == 4, u"IDelegate の呼び出し結果が不正です");

	auto moved_delegate = std::move(delegate);
	NOX_ASSERT(!delegate && moved_delegate, u"MoveOnlyDelegate のムーブ状態が不正です");
	moved_delegate(5);
	NOX_ASSERT(invocation_count == 9, u"MoveOnlyDelegate のムーブ後の呼び出し結果が不正です");

	CopyableDelegate<int(int)> copyable_delegate{ [](int value) noexcept { return value * 2; } };
	auto copied_delegate = copyable_delegate;
	NOX_ASSERT(copyable_delegate(3) == 6 && copied_delegate(4) == 8, u"CopyableDelegate のコピー結果が不正です");
}