///	@file	assertion.cpp
///	@brief	assertion
#include	"assertion.h"
#include	"stdafx.h"

#include	<cassert>
#include	<stacktrace>

#include	"memory/stl_allocate_adapter.h"
#include	"os/mutex.h"
#include	"preprocessor/util.h"
#include	"unicode_converter.h"

#include	"algorithm.h"
#include	"stack_trace.h"
#include	"string_format.h"

using namespace nox;
using namespace nox::debug;

namespace
{
	os::Mutex kMutex;

	inline constexpr std::u16string_view GetRuntimeAssertErrorTypeName(const RuntimeAssertErrorType type)
	{
		constexpr std::array< std::u16string_view, util::ToUnderlying(RuntimeAssertErrorType::_MAX)> table =
		{
			u"",
			u"NullAccess"
		};

		return table.at(util::ToUnderlying(type));
	}
}
void	debug::detail::Assert(RuntimeAssertErrorType errorType, std::u32string_view message, const std::source_location& source_location)noexcept(false)
{
	//	
	std::array<char16, 1024> native_message = { 0 };
	unicode::ConvertU16String(message, native_message);

	std::array<char16, 256> file_name = { 0 };
	unicode::ConvertU16String(source_location.file_name(), file_name);

	std::array<char16, 2048> assert_message = { 0 };
	util::Format(assert_message, u"{0}\n{1}\nLine:{2}, Column:{3}", native_message.data(), file_name.data(), source_location.line(), source_location.column());

	NOX_LOCAL_SCOPE(os::ScopedLock, kMutex);

	::_wassert(util::CharCast<wchar16>(assert_message.data()), util::CharCast<wchar16>(file_name.data()), source_location.line());

//	std::array<wchar16, 1028> conv_message = {L'\0'};
//	nox::unicode::ConvertWString(message.data(), conv_message);
//	AssertImpl(errorType, conv_message.data(), source_location);
}