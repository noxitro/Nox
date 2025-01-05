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

namespace
{
	nox::os::Mutex kMutex;

	inline constexpr std::u16string_view GetRuntimeAssertErrorTypeName(const nox::assertion::RuntimeAssertErrorType type)
	{
		constexpr std::array< std::u16string_view, nox::util::ToUnderlying(nox::assertion::RuntimeAssertErrorType::_MAX)> table =
		{
			u"",
			u"NullAccess"
		};

		return table.at(nox::util::ToUnderlying(type));
	}

	inline constexpr bool is_high_surrogate(const nox::char16 c) { return (c >= 0xD800) && (c < 0xDC00); }

	inline constexpr bool is_low_surrogate(const nox::char16 c) { return (c >= 0xDC00) && (c < 0xE000); }

}

void	nox::assertion::detail::Assert(nox::assertion::RuntimeAssertErrorType errorType, std::u32string_view message, const std::wstring_view file_name, const std::source_location& source_location)noexcept(false)
{
	std::array<char16, 1024> native_message = { 0 };
	unicode::ConvertU16String(message, native_message);

	std::array<char16, 2048> assert_message = { 0 };
	nox::util::Format(assert_message, u"{0}\n{1}\nLine:{2}, Column:{3}", native_message.data(), file_name.data(), source_location.line(), source_location.column());

	NOX_LOCAL_SCOPE(os::ScopedLock, kMutex);
#if! NDEBUG
	::_wassert(nox::util::CharCast<wchar16>(assert_message.data()), file_name.data(), source_location.line());
#endif
//	std::array<wchar16, 1028> conv_message = {L'\0'};
//	nox::unicode::ConvertWString(message.data(), conv_message);
//	AssertImpl(errorType, conv_message.data(), source_location);
}