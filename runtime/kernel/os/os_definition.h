//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	os_definition.h
///	@brief	os_definition
#pragma once
#include	"../basic_type.h"

namespace nox::os
{
	/// @brief ファイルパスの最大長
	constexpr nox::uint16 k_max_path_length = 256;

	/**
	 * @brief オペレーションシステムタイプ
	*/
	enum class OperatingSystemType : uint8
	{
		Win32,
		Win64,
		Android
	};

	/**
	 * @brief スレッドの状態
	*/
	enum class ThreadState : uint8
	{
		/**
		 * @brief 待機中
		*/
		Wait,

		/**
		 * @brief 稼働中
		*/
		Work,

		/**
		 * @brief 終了
		*/
		End
	};

	/**
	 * @brief OS情報
	*/
	struct SystemInfo
	{
		uint8 hardwareNum;
	};

	/**
	 * @brief ミューテックスエラー
	*/
	enum class MutexError : uint8
	{
		None,
		Failed,
	};

	/// @brief メモリオーダーの種類
	enum class MemoryOrder : uint8
	{
		Relaxed,
		Consume,
		Acquire,
		Release,
		AcquireRelease,
		SequentiallyConsistent
	};
}