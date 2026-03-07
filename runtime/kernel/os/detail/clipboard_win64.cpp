//	Copyright (C) 2024 NOX ENGINE All Rights Rserved.

///	@file	clipboard_win64.cpp
///	@brief	clipboard_win64
#include	"stdafx.h"
#include	"clipboard_win64.h"

#include	"log_trace.h"
#include	"assertion.h"
#include	"string_format.h"
#include	"../../log_id.h"
#if NOX_WIN64
#include	"../windows.h"

bool nox::os::clipboard::Clear()
{
	if (::OpenClipboard(nullptr) == FALSE)
	{
		NOX_ERROR_LINE(nox::log_id::OS, u8"OpenClipboard failed.");
		return false;
	}

	if (::EmptyClipboard() == FALSE)
	{
		NOX_ERROR_LINE(nox::log_id::OS, u8"EmptyClipboard failed.");
		return false;
	}

	if (::CloseClipboard() == FALSE)
	{
		NOX_ERROR_LINE(nox::log_id::OS, u8"CloseClipboard failed.");
		return false;
	}
	return true;
}

bool nox::os::clipboard::SetText(const std::u8string_view text)
{
	if (::OpenClipboard(nullptr) == FALSE)
	{
		return false;
	}

	if (::EmptyClipboard() == FALSE)
	{
		NOX_ERROR_LINE(nox::log_id::OS, u8"EmptyClipboard failed.");
		return false;
	}

	::HGLOBAL handle_mem = ::GlobalAlloc(GMEM_MOVEABLE, text.size() + 1);
	if (handle_mem == nullptr)
	{
		NOX_ERROR_LINE(nox::log_id::OS, u8"GlobalAlloc failed.");
		if (::CloseClipboard() == FALSE)
		{
			NOX_ERROR_LINE(nox::log_id::OS, u8"CloseClipboard failed.");
			return false;
		}
		return false;
	}

	::PSTR str_ptr = static_cast<::PSTR>(::GlobalLock(handle_mem));

	const ::errno_t error = ::memcpy_s(str_ptr, (text.size() + 1) , text.data(), text.size() );
	if (error != 0)
	{
		NOX_ERROR_LINE(nox::log_id::OS, u8"memcpy_s failed.");
		::GlobalUnlock(handle_mem);
		::GlobalFree(handle_mem);
		if (::CloseClipboard() == FALSE)
		{
			NOX_ERROR_LINE(nox::log_id::OS, u8"CloseClipboard failed.");
			return false;
		}
		return false;
	}

	::GlobalUnlock(handle_mem);
	::SetClipboardData(CF_TEXT, handle_mem);
	::GlobalFree(handle_mem);
	if (::CloseClipboard() == FALSE)
	{
		NOX_ERROR_LINE(nox::log_id::OS, u8"CloseClipboard failed.");
		return false;
	}
	return true;
}

std::optional<nox::U16String> nox::os::clipboard::GetText()
{
	return std::nullopt;
}

std::optional<nox::U16StringView> nox::os::clipboard::GetText(std::span<nox::char16> dest_buffer)
{
	if (::OpenClipboard(nullptr) == FALSE)
	{
		NOX_ERROR_LINE(nox::log_id::OS, u8"OpenClipboard failed.");
		return std::nullopt;
	}

	return std::nullopt;
}

bool nox::os::detail::ClipboardWin64::Clear()
{
	if (::OpenClipboard(nullptr) == FALSE)
	{
		NOX_ERROR_LINE(nox::log_id::OS, u8"OpenClipboard failed.");
		return false;
	}

	if (::EmptyClipboard() == FALSE)
	{
		NOX_ERROR_LINE(nox::log_id::OS, u8"EmptyClipboard failed.");
		return false;
	}

	if (::CloseClipboard() == FALSE)
	{
		NOX_ERROR_LINE(nox::log_id::OS, u8"CloseClipboard failed.");
		return false;
	}
	return true;
}

bool nox::os::detail::ClipboardWin64::SetText(const std::u8string_view text)
{
	if (::OpenClipboard(nullptr) == FALSE)
	{
		return false;
	}

	if (::EmptyClipboard() == FALSE)
	{
		NOX_ERROR_LINE(nox::log_id::OS, u8"EmptyClipboard failed.");
		return false;
	}

	::HGLOBAL handle_mem = ::GlobalAlloc(GMEM_MOVEABLE, (text.size() + 1) * sizeof(decltype(text)::value_type));
	if (handle_mem == nullptr)
	{
		NOX_ERROR_LINE(nox::log_id::OS, u8"GlobalAlloc failed.");
		::CloseClipboard();
		return false;
	}

	::memcpy_s(::GlobalLock(handle_mem), (text.size() + 1) * sizeof(decltype(text)::value_type), text.data(), text.size() * sizeof(decltype(text)::value_type));
	::GlobalUnlock(handle_mem);
	::SetClipboardData(CF_TEXT, handle_mem);

	return true;
}

#endif // NOX_WIN64
