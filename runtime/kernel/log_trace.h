///	@file	log_trace.h
///	@brief	log_trace
#pragma once

#include	"advanced_type.h"
#include	"convert_string.h"
#include	"nox_string_view.h"


namespace nox::debug
{
	/// @brief ログタイプ
	enum class LogCategory : uint8
	{
		Info,
		Warning,
		Error,
		_Max
	};

	namespace detail
	{
		void	TraceDirect(LogCategory log_category, const StringView category, const StringView message, bool isNewLine, const std::source_location& source_location);
	}

	inline	void	LogTrace(LogCategory log_category, const StringView category, const StringView message, const std::source_location source_location = std::source_location::current())
	{
		nox::debug::detail::TraceDirect(log_category, category, message, true, source_location);
	}

	inline	void	LogTrace(LogCategory log_category, const StringView message, const std::source_location source_location = std::source_location::current())
	{
		nox::debug::LogTrace(log_category, U"unknown", message, source_location);
	}
}

#if NOX_DEBUG
#define	NOX_INFO_LINE(...) ::nox::debug::LogTrace(::nox::debug::LogCategory::Info, __VA_ARGS__)

#else
#define	NOX_INFO_LINE(...)

#endif // NOX_DEBUG
