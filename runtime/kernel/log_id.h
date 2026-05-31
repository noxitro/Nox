//	Copyright (C) 2025 NOX ENGINE All rights reserved.

///	@file	log_id.h
///	@brief	log_id
#pragma once
#include	"log_trace.h"

namespace nox::log_id
{
	struct Kernel : nox::log_id::LogId
	{
		inline constexpr std::u8string_view operator()() const noexcept { return u8"Kernel"; }
	};

	struct Memory : nox::log_id::LogId
	{
		inline constexpr std::u8string_view operator()() const noexcept { return u8"Memory"; }
	};

	struct OS : nox::log_id::LogId
	{
		inline constexpr std::u8string_view operator()() const noexcept { return u8"OS"; }
	};

	struct Reflection : nox::log_id::LogId
	{
		inline constexpr std::u8string_view operator()() const noexcept { return u8"Reflection"; }
	};
}