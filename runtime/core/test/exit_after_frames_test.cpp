//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	exit_after_frames_test.cpp
///	@brief	--exit-after-frames (指定したフレーム数で自動終了する) の引数の読み方の検証。
///	@details	nox::ResolveExitAfterFrames は引数列だけを見る純粋関数として core 側に切り出してある。
///				nox::os のコマンドライン取得に触れないので、World を組み立てず、
///				プロセスの実引数にも依存せずに全分岐を踏める。
///
///				実際にそのフレーム数で終了できるかは、CI の「Run runtime.exe」ステップが
///				runtime.exe を 6 構成すべてで起動して確かめる。

#include	"pch.h"

//	core_test の pch.h は gtest しか載せていない (test_new_delete.cpp の都合)。
//	core のヘッダは kernel / reflection の基盤型に依存するので、main.cpp と同じ順で先に入れる。
#include	"../../kernel/kernel.h"
#include	"../../reflection/reflection.h"

#include	"../world.h"

namespace
{
	[[nodiscard]] nox::uint32 Resolve(const std::initializer_list<const nox::char16*> args)noexcept
	{
		return nox::ResolveExitAfterFrames(std::span<const nox::char16* const>(args.begin(), args.size()));
	}
}

///	@brief	指定が無ければ 0 (自動で終了しない)。普段の起動はこれ。
TEST(ExitAfterFrames, NoArgumentMeansNeverExit)
{
	EXPECT_EQ(Resolve({}), 0u);
	EXPECT_EQ(Resolve({ u"--studio", u"--project-dir=C:\\nox" }), 0u);
}

///	@brief	--exit-after-frames=N はそのまま N になる。区切りは = でも : でもよい。
TEST(ExitAfterFrames, ExplicitFrameCountIsUsed)
{
	EXPECT_EQ(Resolve({ u"--exit-after-frames=120" }), 120u);
	EXPECT_EQ(Resolve({ u"--exit-after-frames:1" }), 1u);
	//	他の引数に埋もれていても拾う。
	EXPECT_EQ(Resolve({ u"--studio", u"--exit-after-frames=60", u"--serial-updater" }), 60u);
}

///	@brief	0 は「自動で終了しない」で、指定が無いときと同じ。
TEST(ExitAfterFrames, ZeroMeansNeverExit)
{
	EXPECT_EQ(Resolve({ u"--exit-after-frames=0" }), 0u);
}

///	@brief	値が無い・空・数字以外を含むときは 0 に落とす。
///	@details	打ち間違いで即座に終了するより、終了しない方が CI の制限時間で気付ける。
TEST(ExitAfterFrames, MalformedValueMeansNeverExit)
{
	EXPECT_EQ(Resolve({ u"--exit-after-frames" }), 0u);
	EXPECT_EQ(Resolve({ u"--exit-after-frames=" }), 0u);
	EXPECT_EQ(Resolve({ u"--exit-after-frames=12x" }), 0u);
	EXPECT_EQ(Resolve({ u"--exit-after-frames=-5" }), 0u);
	EXPECT_EQ(Resolve({ u"--exit-after-frames= 5" }), 0u);
}

///	@brief	名前の似た別の引数は拾わない。
TEST(ExitAfterFrames, SimilarLookingArgumentIsNotMatched)
{
	EXPECT_EQ(Resolve({ u"--exit-after-frames-x=5" }), 0u);
	EXPECT_EQ(Resolve({ u"--exit-after=5" }), 0u);
}

///	@brief	大きすぎる値は nox::kMaxExitAfterFrames で頭打ちにする (uint32 の桁あふれ対策)。
TEST(ExitAfterFrames, TooLargeValueIsClampedToMax)
{
	EXPECT_EQ(Resolve({ u"--exit-after-frames=99999999999999999999" }), nox::kMaxExitAfterFrames);
}
