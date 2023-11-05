///	@file	assertion.cpp
///	@brief	assertion
#include	"stdafx.h"
#include	"assertion.h"
#include	"os/os.h"

#include	<cassert>
#include	<stacktrace>
using namespace nox;
using namespace nox::debug;

namespace
{
	os::Mutex kMutex;
}

void	debug::detail::Assert(not_null<const c16*> message, not_null<const c16*> file, const u32 line)
{
	NOX_LOCAL_SCOPE(os::ScopedLock(kMutex));

	::_wassert(util::CharCast<const wchar_t*>(message), util::CharCast<const wchar_t*>(file.get()), line);
}