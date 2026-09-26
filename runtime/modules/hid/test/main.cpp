//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	main.cpp
///	@brief	hid_test のエントリポイント
///	@details	モジュールは core の上に載るので、core_test と同じく
///				nox::memory と nox::reflection を初期化してからテストを走らせる。
///				順序は runtime の nox::EntryPoint と揃えてある。
#include	"pch.h"

#include	"../../../kernel/kernel.h"
#include	"../../../reflection/reflection.h"

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
