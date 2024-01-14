///	@file	log_trace.h
///	@brief	log_trace
#pragma once

#include	"advanced_type.h"
#include	"convert_string.h"


namespace nox::debug
{
	namespace detail
	{
		enum class LogCategory : uint8
		{
			Info,
			Warning,
			Error
		};

		void	TraceDirect(LogCategory log_category, std::u16string_view category, const U16String& message, bool isNewLine, const c16* file, uint32 line);
	}


	inline	void	InfoLine(const U16String& message, const char16* const file = util::CharCast<const char16*>(__FILEW__), const uint32 line = __LINE__)
	{
		nox::debug::detail::TraceDirect(debug::detail::LogCategory::Info, u"unknown", message, true, file, line);
	}
}