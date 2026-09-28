//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	exit_after_frames_test.cpp
///	@brief	--exit-after-frames (指定したフレーム数で自動終了する) の引数の読み方と、終了要求を出すフレームの検証。
///	@details	nox::ResolveExitAfterFrames は引数列だけを見る純粋関数として core 側に切り出してある。
///				nox::os のコマンドライン取得に触れないので、World を組み立てず、
///				プロセスの実引数にも依存せずに全分岐を踏める。
///
///				終了要求を出すかの判定 nox::ShouldRequestExitAfterFrames も同じく純粋関数で、
///				World::Update が Update フェーズの後・フレーム数を数える前に呼ぶ
///				(旧 SceneManager::Update と同じ位置。N 回目の呼び出しでフレーム数は N - 1)。
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

namespace
{
	/// @brief World::Update の呼び出しを真似て、終了要求が初めて出るのが何回目の呼び出しかを返す。
	/// @details World::Update は「Update フェーズ → 判定 → フレーム数を数える」の順なので、
	///          i 回目 (1 始まり) の呼び出しで判定に渡るフレーム数は i - 1。要求は 1 回だけ出す。
	///          max_calls 回までに出なければ 0。
	[[nodiscard]] nox::uint32 FindRequestCall(const nox::uint32 exit_after_frames, const nox::uint32 max_calls)noexcept
	{
		bool requested = false;
		nox::uint32 request_call = 0u;
		nox::uint32 frame_count = 0u;
		for (nox::uint32 call = 1u; call <= max_calls; ++call)
		{
			if ((requested == false) && nox::ShouldRequestExitAfterFrames(exit_after_frames, frame_count))
			{
				requested = true;
				request_call = call;
			}
			++frame_count;
		}
		return request_call;
	}
}

///	@brief	N を指定したら、World::Update の N 回目の呼び出しで終了要求を出す。
TEST(ExitAfterFrames, RequestIsIssuedOnNthUpdate)
{
	EXPECT_EQ(FindRequestCall(1u, 10u), 1u);
	EXPECT_EQ(FindRequestCall(3u, 10u), 3u);
	EXPECT_EQ(FindRequestCall(10u, 10u), 10u);
	//	N 回目より前には出ない。
	EXPECT_EQ(FindRequestCall(11u, 10u), 0u);
}

///	@brief	判定に渡すフレーム数は「このフレームを数える前の値」。N 回目の呼び出しでは N - 1。
TEST(ExitAfterFrames, FrameCountBeforeIncrementIsCompared)
{
	EXPECT_FALSE(nox::ShouldRequestExitAfterFrames(3u, 0u));
	EXPECT_FALSE(nox::ShouldRequestExitAfterFrames(3u, 1u));
	EXPECT_TRUE(nox::ShouldRequestExitAfterFrames(3u, 2u));
	//	過ぎた後も true のまま。1 回だけ出すのは呼ぶ側 (World) の役目。
	EXPECT_TRUE(nox::ShouldRequestExitAfterFrames(3u, 3u));
	EXPECT_TRUE(nox::ShouldRequestExitAfterFrames(1u, 0u));
}

///	@brief	0 (自動で終了しない) なら、何フレーム回しても要求を出さない。
TEST(ExitAfterFrames, ZeroNeverRequests)
{
	EXPECT_FALSE(nox::ShouldRequestExitAfterFrames(0u, 0u));
	EXPECT_FALSE(nox::ShouldRequestExitAfterFrames(0u, 1000u));
	EXPECT_FALSE(nox::ShouldRequestExitAfterFrames(0u, std::numeric_limits<nox::uint32>::max()));
	EXPECT_EQ(FindRequestCall(0u, 100u), 0u);
}

///	@brief	境界の値でも桁あふれしない (frame_count + 1 ではなく N - 1 と比べている)。
TEST(ExitAfterFrames, NoOverflowAtBoundary)
{
	EXPECT_TRUE(nox::ShouldRequestExitAfterFrames(1u, std::numeric_limits<nox::uint32>::max()));
	EXPECT_FALSE(nox::ShouldRequestExitAfterFrames(nox::kMaxExitAfterFrames, nox::kMaxExitAfterFrames - 2u));
	EXPECT_TRUE(nox::ShouldRequestExitAfterFrames(nox::kMaxExitAfterFrames, nox::kMaxExitAfterFrames - 1u));
}
