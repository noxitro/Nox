//	Copyright (C) 2024 NOX ENGINE All Rights Rserved.

///	@file	os_utility.h
///	@brief	os_utility
#pragma once
#include	"os_definition.h"

namespace nox::os
{
	/**
	 * @brief
	 * @return
	*/
	SystemInfo	GetSystemInfo();

	/**
	 * @brief コア数を取得
	 * @return
	*/
	uint8 GetHardwareConcurrency();
}