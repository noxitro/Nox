//	Copyright (C) 2025 NOX ENGINE All rights reserved.

///	@file	scope_profile.h
///	@brief	scope_profile
#pragma once
#include	"stop_watch.h"
#include	"advanced_type.h"
#include	"utility.h"

namespace nox::util
{
	struct ScopeProfile : INewDeleteDisabled
	{
		ScopeProfile(const std::u16string_view label);
		~ScopeProfile();

		static void* operator new(std::size_t) = delete;
		static void* operator new[](std::size_t) = delete;
	private:
		nox::StopWatch stop_watch_;
		std::array<nox::char16, 256> label_;
	};
}