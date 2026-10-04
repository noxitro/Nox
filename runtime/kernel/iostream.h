//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	iostream.h
///	@brief	iostream
#pragma once
#include	<fstream>
#include	"basic_type.h"

namespace nox::io
{
	using u8ifstream = std::basic_ifstream<nox::char8, std::char_traits<nox::char8>>;
}