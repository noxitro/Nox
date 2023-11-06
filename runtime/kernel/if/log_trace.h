///	@file	log_trace.h
///	@brief	log_trace
#pragma once

#include	"advanced_type.h"
#include	"convert_string.h"


namespace nox::debug
{
	namespace detail
	{
		enum class LogCategory : u8
		{
			Info,
			Warning,
			Error
		};

		void	TraceDirect(LogCategory log_category, std::u16string_view category, const U16String& message, bool isNewLine, const c16* file, u32 line);
	}


	inline	void	InfoLine(const U16String& message, const c16* const file = util::CharCast<const c16*>(__FILEW__), const u32 line = __LINE__) 
	{
		nox::debug::detail::TraceDirect(debug::detail::LogCategory::Info, u"unknown", message, true, file, line);
	}
}