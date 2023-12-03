///	@file	assertion.cpp
///	@brief	assertion
#include	"stdafx.h"
#include	"assertion.h"

#include	<cassert>
#include	<stacktrace>

#include	"os/os.h"
#include	"convert_string.h"

using namespace nox;
using namespace nox::debug;

namespace
{
	os::Mutex kMutex;
}

void	debug::detail::Assert(RuntimeAssertErrorType errorType, not_null<const c16*> message, const std::source_location& source_location)noexcept(false)
{
	NOX_LOCAL_SCOPE(os::ScopedLock(kMutex));


	std::array<wchar_t, 256> file_name;
	util::ConvertWString(file_name, util::CharCast<const c8*>(source_location.file_name()));

	::_wassert(util::CharCast<const wchar_t*>(message), file_name.data(), source_location.line());
}