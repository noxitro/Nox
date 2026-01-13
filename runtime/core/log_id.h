//	Copyright (C) 2025 NOX ENGINE All rights reserved.

///	@file	log_id.h
///	@brief	log_id
#pragma once

namespace nox::log_id
{
	struct CoreCommon : public nox::log_id::LogId
	{
		inline constexpr std::u16string_view operator()() const noexcept { return u"CoreCommon"; }
	};
}