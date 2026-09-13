//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	updater_worker_count_test.cpp
///	@brief	UpdaterGraph のワーカー数をコマンドラインから決める規則の検証。
///	@details	nox::ResolveUpdaterWorkerCount は
///				「引数列 + 既定値」だけを見る純粋関数として core 側に切り出してある。
///				nox::os のコマンドライン取得にも nox::JobSystem の既定値にも触れないので、
///				World を組み立てず、プロセスの実引数にも依存せずに全分岐を踏める。
///
///				既定値を引数で受けるため、テストは論理プロセッサ数に依存しない。
///				既定へ落ちたことを見たい箇所では、ありえない番兵値を渡して同一性で確かめる。
///
///	@note		この関数は nox::World の外にある自由関数で、World の公開範囲は広げていない。
///				World 側は nox::World::World() のメンバ初期化子から
///				  nox::ResolveUpdaterWorkerCount(nox::os::GetCommandLineArgList(),
///				                                 nox::JobSystem::GetDefaultWorkerCount())
///				の形で1回だけ呼ぶ。

#include	"pch.h"

//	core_test の pch.h は gtest しか載せていない (test_new_delete.cpp の都合)。
//	core のヘッダは kernel / reflection の基盤型に依存するので、main.cpp と同じ順で先に入れる。
#include	"../../kernel/kernel.h"
#include	"../../reflection/reflection.h"

#include	"../world.h"

namespace
{
	/// @brief 既定へ落ちたことを検出するための番兵。
	/// @details 実在しうるワーカー数と衝突しない値にしてある
	///          (nox::JobSystem::k_max_worker_count は 63)。
	constexpr nox::uint32 k_default_sentinel = 12345u;

	[[nodiscard]] nox::uint32 Resolve(const std::initializer_list<const nox::char16*> args)noexcept
	{
		return nox::ResolveUpdaterWorkerCount(
			std::span<const nox::char16* const>(args.begin(), args.size()),
			k_default_sentinel);
	}
}

///	@brief	指定が無ければ既定値がそのまま返る。
TEST(UpdaterWorkerCount, NoArgumentFallsBackToDefault)
{
	EXPECT_EQ(Resolve({}), k_default_sentinel);
	EXPECT_EQ(Resolve({ u"--project-dir=C:\\nox", u"--something-else" }), k_default_sentinel);
}

///	@brief	--serial-updater は0本(直列)。
TEST(UpdaterWorkerCount, SerialUpdaterMeansZeroWorkers)
{
	EXPECT_EQ(Resolve({ u"--serial-updater" }), 0u);
	EXPECT_EQ(Resolve({ u"--project-dir=C:\\nox", u"--serial-updater" }), 0u);
}

///	@brief	--serial-updater は --updater-workers=N より強い。並び順にも依存しない。
TEST(UpdaterWorkerCount, SerialUpdaterWinsOverExplicitWorkerCount)
{
	EXPECT_EQ(Resolve({ u"--serial-updater", u"--updater-workers=8" }), 0u);
	EXPECT_EQ(Resolve({ u"--updater-workers=8", u"--serial-updater" }), 0u);
}

///	@brief	--updater-workers=N はそのままNになる。
TEST(UpdaterWorkerCount, ExplicitWorkerCountIsUsed)
{
	EXPECT_EQ(Resolve({ u"--updater-workers=4" }), 4u);
	EXPECT_EQ(Resolve({ u"--updater-workers=1" }), 1u);
	EXPECT_EQ(Resolve({ u"--updater-workers=31" }), 31u);
	//	他の引数に埋もれていても拾う。
	EXPECT_EQ(Resolve({ u"--project-dir=C:\\nox", u"--updater-workers=2", u"--vsync" }), 2u);
}

///	@brief	--updater-workers=0 は直列。--serial-updater と同じ意味になる。
///	@details	既定へ落とす選択肢もあるが、0を「ワーカー0本」と読むのが素直で、
///				nox::JobSystem::Initialize(0) が「スレッドを1本も作らない」を
///				正式に受け付ける仕様とも一致する。
///				既定へのフォールバックは「値が壊れているとき」だけに限定する。
TEST(UpdaterWorkerCount, ZeroIsSerialNotDefault)
{
	EXPECT_EQ(Resolve({ u"--updater-workers=0" }), 0u);
	EXPECT_NE(Resolve({ u"--updater-workers=0" }), k_default_sentinel);
}

///	@brief	数字でない文字が混ざっていたら既定へ落とす。
///	@details	黙って0本(=直列)にすると、打ち間違いが「なぜか遅い」として表れて原因が見えない。
TEST(UpdaterWorkerCount, MalformedValueFallsBackToDefault)
{
	EXPECT_EQ(Resolve({ u"--updater-workers=abc" }), k_default_sentinel);
	EXPECT_EQ(Resolve({ u"--updater-workers=4x" }), k_default_sentinel);
	EXPECT_EQ(Resolve({ u"--updater-workers=-1" }), k_default_sentinel);
	EXPECT_EQ(Resolve({ u"--updater-workers= 4" }), k_default_sentinel);
	//	値そのものが無い場合も既定へ。
	EXPECT_EQ(Resolve({ u"--updater-workers=" }), k_default_sentinel);
}

///	@brief	上限を超えた指定は k_max_worker_count へ丸められる。
///	@details	丸めは方針ではなく桁あふれ対策。解析の途中で打ち切るので、
///				uint32 を回り込むような長い数字列でも上限に収まる。
TEST(UpdaterWorkerCount, TooLargeValueIsClampedToMax)
{
	static_assert(nox::JobSystem::k_max_worker_count == 63u,
		"下の 63 / 64 の境界テストは k_max_worker_count == 63 を前提にしている");

	EXPECT_EQ(Resolve({ u"--updater-workers=64" }), nox::JobSystem::k_max_worker_count);
	EXPECT_EQ(Resolve({ u"--updater-workers=1000" }), nox::JobSystem::k_max_worker_count);
	//	uint32 を優に超える桁数。回り込んで小さい値になってはいけない。
	EXPECT_EQ(Resolve({ u"--updater-workers=99999999999999999999" }), nox::JobSystem::k_max_worker_count);
	//	上限ちょうどは丸めの影響を受けない。
	EXPECT_EQ(Resolve({ u"--updater-workers=63" }), 63u);
}

///	@brief	キーの前方一致で別の引数を巻き込まない。
///	@details	キーに '=' を含めているので "--updater-workersXXX" は拾わない。
TEST(UpdaterWorkerCount, SimilarLookingArgumentIsNotMatched)
{
	EXPECT_EQ(Resolve({ u"--updater-workers" }), k_default_sentinel);
	EXPECT_EQ(Resolve({ u"--updater-workers-extra=4" }), k_default_sentinel);
}

///	@brief	同じキーが複数あれば先に現れた方を使う。
///	@details	nox::os::GetCommandLineArgValue と同じ「最初の一致」の流儀に揃えてある。
TEST(UpdaterWorkerCount, FirstMatchWins)
{
	EXPECT_EQ(Resolve({ u"--updater-workers=3", u"--updater-workers=7" }), 3u);
}

///	@brief	nullptr が混ざっていても落ちない。
///	@details	引数列は生ポインタの並びなので、空きが混ざる可能性を潰しておく。
TEST(UpdaterWorkerCount, NullArgumentIsSkipped)
{
	EXPECT_EQ(Resolve({ nullptr, u"--updater-workers=5", nullptr }), 5u);
	EXPECT_EQ(Resolve({ nullptr }), k_default_sentinel);
}
