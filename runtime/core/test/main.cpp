//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	main.cpp
///	@brief	core_test のエントリポイント
///	@details	core のテストは kernel_test と違い、
///				nox::memory と nox::reflection の初期化を前提にする
///				(リフレクションのデータベースは nox::reflection::Initialize() で
///				生成コードの登録関数を呼んで初めて引ける)。
///				順序は runtime の nox::EntryPoint と揃えてある。

#include	"pch.h"

#include	"../../kernel/kernel.h"
#include	"../../reflection/reflection.h"

int main(int argc, char** argv)
{
	::testing::InitGoogleTest(&argc, argv);

	nox::memory::Initialize(std::numeric_limits<nox::int32>::max(), true);
	nox::reflection::Initialize();

	const int result = RUN_ALL_TESTS();

	nox::reflection::Finalize();
	nox::memory::ReleaseBootMemory();
	nox::memory::Finialize();

	return result;
}
