//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	studio_mode_test.cpp
///	@brief	--studio (Editor から起動された実行) の判定規則の検証。
///	@details	nox::ResolveStudioMode は引数列だけを見る純粋関数として core 側に切り出してある
///				(World と nox::SceneManager が同じ規則で読むため)。
///				nox::os のコマンドライン取得に触れないので、World を組み立てず、
///				プロセスの実引数にも依存せずに全分岐を踏める。

#include	"pch.h"

//	core_test の pch.h は gtest しか載せていない (test_new_delete.cpp の都合)。
//	core のヘッダは kernel / reflection の基盤型に依存するので、main.cpp と同じ順で先に入れる。
#include	"../../kernel/kernel.h"
#include	"../../reflection/reflection.h"

#include	"../world.h"

namespace
{
	[[nodiscard]] bool Resolve(const std::initializer_list<const nox::char16*> args)noexcept
	{
		return nox::ResolveStudioMode(std::span<const nox::char16* const>(args.begin(), args.size()));
	}
}

///	@brief	指定が無ければ studio mode ではない。普段の起動はこれ。
TEST(StudioMode, NoArgumentMeansNotStudio)
{
	EXPECT_FALSE(Resolve({}));
	EXPECT_FALSE(Resolve({ u"--project-dir=C:\nox", u"--exit-after-frames=60" }));
}

///	@brief	--studio があれば studio mode。他の引数に埋もれていても拾う。
TEST(StudioMode, StudioArgumentIsDetected)
{
	EXPECT_TRUE(Resolve({ u"--studio" }));
	EXPECT_TRUE(Resolve({ u"--project-dir=C:\nox", u"--studio", u"--serial-updater" }));
}

///	@brief	値は見ない。キーがあれば studio mode (nox::os::ContainsCommandLineArgKey と同じ規則)。
TEST(StudioMode, ValueIsIgnored)
{
	EXPECT_TRUE(Resolve({ u"--studio=1" }));
	EXPECT_TRUE(Resolve({ u"--studio:" }));
}

///	@brief	名前の似た別の引数は拾わない。
TEST(StudioMode, SimilarLookingArgumentIsNotMatched)
{
	EXPECT_FALSE(Resolve({ u"--studiox" }));
	EXPECT_FALSE(Resolve({ u"--studio-mode" }));
	EXPECT_FALSE(Resolve({ u"-studio" }));
}

///	@brief	nullptr が混ざっていても落ちない。
TEST(StudioMode, NullArgumentIsSkipped)
{
	EXPECT_TRUE(Resolve({ nullptr, u"--studio", nullptr }));
	EXPECT_FALSE(Resolve({ nullptr }));
}
