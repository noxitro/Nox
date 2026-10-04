// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

/// @file	file_base.h
/// @brief	file_base
#pragma once
#include	<concepts>
#include	"../advanced_type.h"

namespace nox::detail
{
	class FileBase
	{

	};

	namespace concepts
	{
		template<class T>
		concept File = std::derived_from<T, nox::detail::FileBase>;
	}
}