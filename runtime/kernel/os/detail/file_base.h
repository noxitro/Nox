// Copyright (C) 2026 NOX ENGINE All rights reserved.

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