//	Copyright (C) 2024 NOX ENGINE All Rights Rserved.

///	@file	clipboard_win64.cpp
///	@brief	clipboard_win64
#include	"stdafx.h"
#include	"clipboard_win64.h"

#include	"log_trace.h"
#include	"assertion.h"
#include	"string_format.h"

#if NOX_WIN64
#include	"../windows.h"

namespace
{
	class ScopeExit
	{
	public:
		inline constexpr explicit ScopeExit(void(*&func)())noexcept:
			func_(func)
		{

		}

		inline ~ScopeExit()
		{
			func_();
		}

	private:
		void(*func_)();
	};
}

bool nox::os::clipboard::Clear()
{
	if (::OpenClipboard(nullptr) == FALSE)
	{
		NOX_ERROR_LINE(U"OpenClipboard failed.");
		return false;
	}

	if (::EmptyClipboard() == FALSE)
	{
		NOX_ERROR_LINE(U"EmptyClipboard failed.");
		return false;
	}

	if (::CloseClipboard() == FALSE)
	{
		NOX_ERROR_LINE(U"CloseClipboard failed.");
		return false;
	}
	return true;
}

bool nox::os::clipboard::SetText(const nox::StringView text)
{
	std::array<nox::char8, 1024> buffer = { 0 };
	NOX_ASSERT(text.size() < buffer.size(), nox::util::Format(U"buffer size over:{0}, max:{1}", text.size(), buffer.size()));
	return nox::os::clipboard::SetText(nox::unicode::ConvertU8String(text, buffer));
}

bool nox::os::clipboard::SetText(const std::u8string_view text)
{
	if (::OpenClipboard(nullptr) == FALSE)
	{
		return false;
	}

	if (::EmptyClipboard() == FALSE)
	{
		NOX_ERROR_LINE(U"EmptyClipboard failed.");
		return false;
	}

	::HGLOBAL handle_mem = ::GlobalAlloc(GMEM_MOVEABLE, text.size() + 1);
	if (handle_mem == nullptr)
	{
		NOX_ERROR_LINE(U"GlobalAlloc failed.");
		if (::CloseClipboard() == FALSE)
		{
			NOX_ERROR_LINE(U"CloseClipboard failed.");
			return false;
		}
		return false;
	}

	::PSTR str_ptr = static_cast<::PSTR>(::GlobalLock(handle_mem));

	const ::errno_t error = ::memcpy_s(str_ptr, (text.size() + 1) , text.data(), text.size() );
	if (error != 0)
	{
		NOX_ERROR_LINE(U"memcpy_s failed.");
		::GlobalUnlock(handle_mem);
		::GlobalFree(handle_mem);
		if (::CloseClipboard() == FALSE)
		{
			NOX_ERROR_LINE(U"CloseClipboard failed.");
			return false;
		}
		return false;
	}

	::GlobalUnlock(handle_mem);
	::SetClipboardData(CF_TEXT, handle_mem);
	::GlobalFree(handle_mem);
	if (::CloseClipboard() == FALSE)
	{
		NOX_ERROR_LINE(U"CloseClipboard failed.");
		return false;
	}
	return true;
}

std::optional<nox::String> nox::os::clipboard::GetText()
{
	return std::nullopt;
}

std::optional<nox::StringView> nox::os::clipboard::GetText(std::span<nox::char32> dest_buffer)
{
	if (::OpenClipboard(nullptr) == FALSE)
	{
		NOX_ERROR_LINE(U"OpenClipboard failed.");
		return std::nullopt;
	}

	return std::nullopt;
}

bool nox::os::detail::ClipboardWin64::Clear()
{
	if (::OpenClipboard(nullptr) == FALSE)
	{
		NOX_ERROR_LINE(U"OpenClipboard failed.");
		return false;
	}

	if (::EmptyClipboard() == FALSE)
	{
		NOX_ERROR_LINE(U"EmptyClipboard failed.");
		return false;
	}

	if (::CloseClipboard() == FALSE)
	{
		NOX_ERROR_LINE(U"CloseClipboard failed.");
		return false;
	}
	return true;
}

bool nox::os::detail::ClipboardWin64::SetText(const nox::StringView text)
{
	if (::OpenClipboard(nullptr) == FALSE)
	{
		return false;
	}

	if (::EmptyClipboard() == FALSE)
	{
		NOX_ERROR_LINE(U"EmptyClipboard failed.");
		return false;
	}

	::HGLOBAL handle_mem = ::GlobalAlloc(GMEM_MOVEABLE, (text.size() + 1) * sizeof(nox::StringView::value_type));
	if (handle_mem == nullptr)
	{
		NOX_ERROR_LINE(U"GlobalAlloc failed.");
		::CloseClipboard();
		return false;
	}

	::memcpy_s(::GlobalLock(handle_mem), (text.size() + 1) * sizeof(nox::StringView::value_type), text.data(), text.size() * sizeof(nox::StringView::value_type));
	::GlobalUnlock(handle_mem);
	::SetClipboardData(CF_TEXT, handle_mem);

	return true;
}

#endif // NOX_WIN64
