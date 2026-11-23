//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	assertion_id.cpp
///	@brief	assertion_id
#include	"pch.h"
#include	"assertion_id.h"

#include	"string_format.h"

void nox::assertion::OutOfRange::Format(std::span<char8_t> dest)const
{
	nox::util::Format(dest, u8"Index {0} is out of range (range:{1})", index, range);
}
