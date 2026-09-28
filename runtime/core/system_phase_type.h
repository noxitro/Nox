//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	system_phase_type.h
///	@brief	UpdaterGraph のフェーズ
#pragma once

namespace nox
{
	/// @brief		UpdaterGraph のフェーズ。ノード(EntitySystem / EntityLogic / Serviceのメソッド / Task)は
	///				どれか1つのフェーズに属し、フェーズの中の順序は UpdaterGraph が宣言から決める。
	/// @details	フェーズをまたぐ順序は nox::World が呼ぶ順で決まる。ゲームスレッドは Init → Start の後、
	///				毎フレーム FrameIngress → Update → Presentation を回し、終了時に Terminate を1回回す。
	///				各フェーズの末尾で EntityCommands が反映されるので、あるフェーズで積んだ構造変更は次のフェーズから見える。
	///
	///				Service の寿命(OnInitialize / OnShutdown)はフェーズではない。OnInitialize は World::Init で
	///				Init フェーズより前に、OnShutdown は World::Exit で Terminate フェーズより後に呼ばれる。
	enum class SystemPhaseType : nox::uint8
	{
		/// @brief ゲームスレッドの開始時に1回だけ実行されるフェーズ。Service は初期化済み。
		Init = 0,
		/// @brief Init の後、最初のフレームの前に1回だけ実行されるフェーズ。
		Start,
		/// @brief 毎フレーム、Update の前に呼び出される外部からの取り込み
		/// @details 入力のポーリング、ソケット受信、アセット完了の取り込みなど、
		///          フレームの外で起きたことをこのフレームの状態へ反映する処理を置く。
		///          末尾で EntityCommands が反映されるので、ここで積んだ構造変更は Update から見える。
		FrameIngress,
		/// @brief 毎フレーム呼び出されるゲームロジック
		Update,
		/// @brief 毎フレーム、Update の後に呼び出される描画の抽出・提出
		/// @details Update で確定したこのフレームの状態を読み、描画へ渡す処理を置く。
		///          末尾で EntityCommands が反映されるので、ここで積んだ構造変更は次のフレームの FrameIngress から見える。
		Presentation,
		/// @brief ゲームスレッドの終了時に1回だけ実行されるフェーズ。この後で Service が終了する。
		Terminate,
		_Max
	};
}