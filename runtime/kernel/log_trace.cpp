///	@file	log_trace.cpp
///	@brief	log_trace
#include	"stdafx.h"
#include	"log_trace.h"
#include	"algorithm.h"
#include	"memory/stl_allocate_adapter.h"
#include	"unicode_converter.h"
#include	"string_format.h"
#include	"os/thread.h"

#if NOX_WINDOWS
#include	"os/windows.h"
#endif // NOX_WIN64


#include	<iostream>

namespace nox
{
	inline constexpr nox::StringView GetLogCategoryName(nox::debug::LogCategory log_category)noexcept
	{
		constexpr std::array<nox::StringView, nox::util::ToUnderlying(nox::debug::LogCategory::_Max)> table =
		{
			u"Info",
			u"Warning",
			u"Error",
		};

		return table.at(nox::util::ToUnderlying(log_category));
	}

//	constinit std::array<nox::char32, 64> log_buffer_table 

	struct ThreadData
	{
		std::array<nox::char16, 5016> log_buffer_table = { 0 };
	};

	/// @brief スレッド分のログバッファ
	constinit std::array<ThreadData, nox::os::MAX_THREAD_ID> thread_data_table = { 0 };
}

void nox::debug::detail::TraceDirect(nox::debug::LogCategory log_category, const std::u32string_view category, const std::u32string_view message, bool isNewLine, const std::source_location& source_location)
{
	std::array<char16, 2048> buffer = { 0 };
	//source_location;
	if (isNewLine)
	{
//		util::Format(buffer, u"[{0}][{1}]{2}\n", GetLogCategoryName(log_category), category.data(), message.data());
	}
	else
	{
	//	util::Format(buffer, u"[{0}][{1}]{2}", GetLogCategoryName(log_category), category.data(), message.data());
	}

	const wchar_t* converted_str = nox::util::CharCast<wchar_t>(buffer.data());

	//	コンソールへの出力
#if NOX_WINDOWS
	//	デバッグウィンドウに出力
	::OutputDebugStringW(converted_str);
#endif // NOX_WIN64

	std::wcout << converted_str ;
}