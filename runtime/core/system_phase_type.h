//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	module_entry_category.h
///	@brief	module_entry_category
#pragma once

namespace nox
{
	enum class SystemPhaseType : nox::uint8
	{
		/// @brief 他のシステムに依存しない初期化処理
		Init = 0,
		/// @brief 更新の前に一度だけ呼び出される処理
		Start,
		/// @brief 毎フレーム、Update の前に呼び出される外部からの取り込み
		/// @details 入力のポーリング、ソケット受信、アセット完了の取り込みなど、
		///          フレームの外で起きたことをこのフレームの状態へ反映する処理を置く。
		///          末尾で EntityCommands が反映されるので、ここで積んだ構造変更は Update から見える。
		FrameIngress,
		/// @brief 毎フレーム呼び出される処理
		Update,
		/// @brief 毎フレーム、Update の後に呼び出される描画の抽出・提出
		/// @details Update で確定したこのフレームの状態を読み、描画へ渡す処理を置く。
		///          末尾で EntityCommands が反映されるので、ここで積んだ構造変更は次のフレームの FrameIngress から見える。
		Presentation,
		/// @brief 終了処理
		Terminate,
		_Max
	};
}