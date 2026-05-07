///	@file	log_trace.cpp
///	@brief	log_trace
#include	"pch.h"
#include	"log_trace.h"
#include	"algorithm.h"
#include	"memory/stl_allocate_adapter.h"
#include	"unicode_converter.h"
#include	"string_format.h"
#include	"os/thread.h"
#include	"ascii.h"

#if NOX_WINDOWS
#include	"os/windows.h"
#endif // NOX_WIN64

#include	<iostream>

namespace nox
{
	namespace
	{
		inline constexpr std::u16string_view GetLogCategoryName(nox::debug::LogLevel log_category)noexcept
		{
			constexpr std::array<std::u16string_view, nox::util::ToUnderlying(nox::debug::LogLevel::_Max)> table =
			{
				u"Info",
				u"Warning",
				u"Error",
			};

			return table.at(nox::util::ToUnderlying(log_category));
		}

		static inline std::function<void(const nox::debug::LogHandlerArgs&)> g_log_handler = nullptr;
	}
}

void nox::debug::AttachLogHandler(std::function<void(const nox::debug::LogHandlerArgs&)> handler)
{
	g_log_handler = handler;
}

void nox::debug::DetachLogHandler()
{
	g_log_handler = nullptr;
}

void nox::debug::detail::TraceDirect(nox::debug::LogLevel log_category, const std::u16string_view channel, const std::u8string_view message, bool isNewLine, const std::source_location& source_location)
{
	std::array<char16, 2048> buffer = { 0 };
	//source_location;
	if (isNewLine)
	{
		nox::util::Format(buffer, u"[{0}][{1}]{2}\n", GetLogCategoryName(log_category), channel.data(), message.data());
	}
	else
	{
		util::Format(buffer, u"[{0}][{1}]{2}", GetLogCategoryName(log_category), channel.data(), message.data());
	}
	
	const wchar_t* converted_str = nox::util::CharCast<wchar_t>(buffer.data());

	//	コンソールへの出力
#if NOX_WINDOWS
	//	デバッグウィンドウに出力
	::OutputDebugStringW(converted_str);
#endif // NOX_WIN64

	std::wcout << converted_str ;

	if (g_log_handler != nullptr)
	{
		std::array<nox::char8, 256> channel_buffer = { 0 };
		const std::u8string_view channnel_stringview = nox::encoding::ascii::ConvertString(channel, std::span(channel_buffer));

		const nox::debug::LogHandlerArgs args{
			.column = source_location.column(),
			.level = log_category,
			.message = message,
			.channel = channnel_stringview,
		};

		g_log_handler(args);
	}
}

void nox::debug::detail::TraceDirect(nox::debug::LogLevel log_category, const std::u16string_view channel, const std::u16string_view message, bool isNewLine, const std::source_location& source_location)
{
	std::array<char16, 2048> buffer = { 0 };
	//source_location;
	if (isNewLine)
	{
		nox::util::Format(buffer, u"[{0}][{1}]{2}\n", GetLogCategoryName(log_category), channel.data(), message.data());
	}
	else
	{
		util::Format(buffer, u"[{0}][{1}]{2}", GetLogCategoryName(log_category), channel.data(), message.data());
	}
	
	const wchar_t* converted_str = nox::util::CharCast<wchar_t>(buffer.data());

	//	コンソールへの出力
#if NOX_WINDOWS
	//	デバッグウィンドウに出力
	::OutputDebugStringW(converted_str);
#endif // NOX_WIN64

	std::wcout << converted_str ;

	if (g_log_handler != nullptr)
	{
		std::array<nox::char8, 256> channel_buffer = { 0 };
		const std::u8string_view channnel_stringview = nox::encoding::ascii::ConvertString(channel, std::span(channel_buffer));

		std::array<nox::char8, 2048> message_buffer = { 0 };
		const std::u8string_view utf8_message = nox::unicode::ConvertU8String(message, message_buffer);
		const nox::debug::LogHandlerArgs args{
			.column = source_location.column(),
			.level = log_category,
			.message = utf8_message,
			.channel = channnel_stringview,
		};
		g_log_handler(args);
	}
}
