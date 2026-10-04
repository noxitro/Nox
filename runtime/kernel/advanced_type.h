// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

#pragma once
//#include	<array>
//#include	<bitset>
//#include	<concepts>
//#include	<functional>
//#include    <optional>
//#include	<set>
//#include	<source_location>
//#include    <span>
//#include	<mdspan>
//#include	<stack>
//#include	<string>
//#include	<string_view>
//#include	<utility>
//#include	<ranges>
//#include	<expected>
//#include	<chrono>
//#include	<shared_mutex>
//#include	<fstream>
//#include	<filesystem>
//#include	<semaphore>
#include	<iostream>

#include	"basic_type.h"

namespace nox
{
	namespace io
	{
		using u8ifstream = std::basic_ifstream<nox::char8, std::char_traits<nox::char8>>;
	}
}