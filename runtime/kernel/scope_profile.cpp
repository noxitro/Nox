//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	scope_profile.cpp
///	@brief	scope_profile
#include	"pch.h"
#include	"scope_profile.h"

#include	"advanced_type.h"
#include	"string_util.h"
#include	"log_trace.h"
#include	"log_id.h"

nox::util::ScopeProfile::ScopeProfile(const std::u16string_view label)
{
	nox::util::StrCopy(label, std::span<nox::char16>(label_));
	stop_watch_.Start();
}

nox::util::ScopeProfile::~ScopeProfile()
{
	const std::u16string_view s = label_.data();
	NOX_INFO_LINE(nox::log_id::Kernel, u"{0}:{1}msec", s, stop_watch_.ElapsedMilliseconds());
}