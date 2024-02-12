///	@file	assertion.cpp
///	@brief	assertion
#include	"assertion.h"
#include	"stdafx.h"

#include	<cassert>
#include	<stacktrace>

#include	"../preprocessor/util.h"
#include	"../os/mutex.h"
#include	"../memory/stl_allocate_adapter.h"
#include	"unicode_converter.h"

using namespace nox;
using namespace nox::debug;

namespace
{
	os::Mutex kMutex;

	void	AssertImpl(RuntimeAssertErrorType errorType, std::wstring_view message, const std::source_location& source_location)noexcept(false)
	{
		NOX_LOCAL_SCOPE(os::ScopedLock(kMutex));

		std::array<wchar_t, 256> file_name;
		nox::unicode::ConvertWString(file_name, util::CharCast<const c8*>(source_location.file_name()));
		::_wassert(message.data(), file_name.data(), source_location.line());
	}
}

void	debug::detail::Assert(RuntimeAssertErrorType errorType, std::u16string_view message, const std::source_location& source_location)noexcept(false)
{
	AssertImpl(errorType, util::CharCast<const wchar_t*>(message.data()), source_location);
}

void	debug::detail::Assert(RuntimeAssertErrorType errorType, std::u32string_view message, const std::source_location& source_location)noexcept(false)
{
	const nox::WString convStr = nox::unicode::ConvertWString(message.data());
	AssertImpl(errorType, convStr.c_str(), source_location);
}