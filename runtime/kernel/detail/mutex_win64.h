//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	mutex_win64.h
///	@brief	mutex_win64
#pragma once
#include	"../mutex.h"
#include	"../win64_api.h"

struct nox::Mutex::Detail final
{
	static ::PSRWLOCK GetNativeLock(nox::Mutex& mutex) noexcept;
};