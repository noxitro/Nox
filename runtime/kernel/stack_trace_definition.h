//	Copyright (C) 2025 NOX ENGINE All rights reserved.

///	@file	stack_trace_definition.h
///	@brief	stack_trace_definition
#pragma once
#include	"basic_type.h"

namespace nox::stack_walker
{
	/// @brief 最大スタック数
	constexpr nox::uint8 MAX_STACK_DEPTH = 32U;
	constexpr nox::uint8 DEFAULT_STACK_DEPTH = 16U;

	namespace detail
	{
		constexpr nox::uint16 kMaxModuleName = 255U;
		constexpr nox::uint16 kMaxFileName = 1024U;
		constexpr nox::uint16 kMaxSymbolName = 255U;
	}
}