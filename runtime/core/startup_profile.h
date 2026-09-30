//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	startup_profile.h
///	@brief	起動の区切りごとの時刻とヒープ確保回数の記録
///	@details	runtime.exe の起動にかかる時間を、エンジン自身の区間ごとに測るためのもの。
///				プロセス全体の壁時計 (起動から終了まで) は、ウィンドウやデバイスの作成、OS のローダが
///				大半を占めて揺れも大きく、エンジンの初期化 (reflection の登録や UpdaterGraph の構築) の
///				変化が埋もれる。そこで区切りごとに QueryPerformanceCounter と確保の累積値を控えておき、
///				CI (.github/scripts/bench-run.py) が区間に分けて比べる。
///
///				記録は固定長の配列へ書くだけで、ヒープも同期も使わない。Master を含む全構成で有効。
///				--startup-report=<パス> を渡したときだけ、終了前に JSON (nox-startup/1) を書き出す。
///
///				core.h からは include しない (ReflectionGenerator の解析対象に入れないため)。
#pragma once
#include	"../kernel/basic_type.h"

#include	<span>

namespace nox::startup_profile
{
	/// @brief 起動の区切り。値は JSON の並び順でもある
	enum class Point : nox::uint8
	{
		/// @brief EntryPoint の入口。ここより前は OS のローダ、DLL の読み込み、静的初期化
		EntryPoint,
		/// @brief nox::memory::Initialize の後
		MemoryInitialized,
		/// @brief nox::reflection::Initialize の後 (生成コードの型登録)
		ReflectionInitialized,
		/// @brief nox::os::Initialize の後
		OsInitialized,
		/// @brief World::Init の後 (モジュールとシステムの生成、UpdaterGraph の構築、JobSystem の起動)
		WorldInitialized,
		/// @brief Init フェーズの後 (ウィンドウやデバイスの作成など、各システムの初期化)
		InitPhaseDone,
		/// @brief Start フェーズの後
		StartPhaseDone,
		/// @brief 最初の Update フェーズを実行し終えたところ
		FirstFrameDone,

		_Max
	};

	/// @brief		区切りを記録する。同じ区切りは最初の 1 回だけを残す
	/// @details	どのスレッドから呼んでもよいが、同じ区切りを複数のスレッドから同時に呼ばないこと。
	///				書き出しは World の終了 (ゲームスレッドの Wait) の後なので、その間の同期は要らない。
	void Mark(nox::startup_profile::Point point)noexcept;

	/// @brief		--startup-report=<パス> が指定されていれば、記録を JSON で書き出す
	/// @details	書き出しに失敗しても落とさない (CI 側が「レポートが無い」として扱う)。
	/// @param command_line_args nox::os::GetCommandLineArgList() と同じ並び
	void WriteReportIfRequested(std::span<const nox::char16* const> command_line_args)noexcept;
}
