//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	os_definition.h
///	@brief	os_definition
#pragma once
#include	"basic_type.h"

namespace nox
{
	/// @brief ファイルパスの最大長
	constexpr nox::uint16 k_max_path_length = 256;

	/// @brief オペレーションシステムタイプ
	enum class OperatingSystemType : nox::uint8
	{
		Win32,
		Win64,
		Android
	};

	/// @brief スレッドの状態
	enum class ThreadState : nox::uint8
	{
		Wait,
		Work,
		Terminated
	};

	/// @brief ミューテックスエラー
	enum class MutexError : nox::uint8
	{
		None,
		Failed,
	};

	/// @brief メモリオーダーの種類
	enum class MemoryOrder : nox::uint8
	{
		Relaxed,
		Consume,
		Acquire,
		Release,
		AcquireRelease,
		SequentiallyConsistent
	};
}

namespace nox::os
{
	/// @brief OS情報
	struct SystemInfo
	{
		nox::uint8 hardwareNum;
	};
}