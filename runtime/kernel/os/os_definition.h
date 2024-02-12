//	Copyright (C) 2024 NOX ENGINE All Rights Rserved.

///	@file	os_definition.h
///	@brief	os_definition
#pragma once
#include	"../if/basic_type.h"

namespace nox::os
{
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
}