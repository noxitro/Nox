// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

/// @file	file_base.h
/// @brief	file_base
#pragma once
#include	"../../advanced_type.h"

namespace nox::os
{
	class FileBase
	{

	};

	namespace concepts
	{
		template<class T>
		concept File = std::derived_from<T, nox::os::FileBase>;
	}
}