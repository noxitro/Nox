// Copyright (C) 2026 NOX ENGINE All rights reserved.

/// @file	core_utility.cpp
/// @brief	core_utility
#include "pch.h"
#include "core_utility.h"

std::u16string_view nox::util::GetProjectDir()noexcept
{
	const auto r = nox::os::GetCommandLineArgValue(u"--project-dir");
	NOX_ASSERT(r.has_value(), u"--project-dir is not specified.");
	return r.value();
}

std::u8string_view nox::util::GetProjectDir(std::array<nox::char8, nox::os::k_max_path_length>& buffer)noexcept
{
	const auto r = nox::os::GetCommandLineArgValue(u"--project-dir");
	NOX_ASSERT(r.has_value(), u"--project-dir is not specified.");
	const std::u16string_view project_dir_u16 = r.value();
	return nox::unicode::ConvertU8String(project_dir_u16, buffer);
}
