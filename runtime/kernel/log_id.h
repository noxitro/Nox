//	Copyright (C) 2025 NOX ENGINE All rights reserved.

///	@file	log_id.h
///	@brief	log_id
#pragma once
#include	"log_trace.h"

namespace nox::log_id
{
	struct Kernel : LogId
	{
		inline constexpr std::u32string_view operator()() const noexcept { return U"Kernel"; }
	};

	struct Memory : LogId
	{
		inline constexpr std::u32string_view operator()() const noexcept { return U"Memory"; }
	};

	struct OS : LogId
	{
		inline constexpr std::u32string_view operator()() const noexcept { return U"OS"; }
	};

	struct Reflection : LogId
	{
		inline constexpr std::u32string_view operator()() const noexcept { return U"Reflection"; }
	};
}