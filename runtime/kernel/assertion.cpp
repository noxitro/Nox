///	@file	assertion.cpp
///	@brief	assertion
#include	"assertion.h"
#include	"pch.h"

#include	<cassert>
#include	<stacktrace>

#include	"memory/stl_allocate_adapter.h"
#include	"os/mutex.h"
#include	"preprocessor/util.h"
#include	"unicode_converter.h"

#include	"algorithm.h"
#include	"stack_trace.h"
#include	"string_format.h"

namespace nox
{
	namespace
	{
		nox::os::Mutex kMutex;

		inline constexpr bool is_high_surrogate(const nox::char16 c) { return (c >= 0xD800) && (c < 0xDC00); }

		inline constexpr bool is_low_surrogate(const nox::char16 c) { return (c >= 0xDC00) && (c < 0xE000); }
	}
}

void	nox::assertion::detail::Assert(std::u16string_view error_category, std::u8string_view message, const std::wstring_view file_name, const std::source_location& source_location)noexcept(false)
{
	std::array<nox::char16, 1024> native_message = { 0 };
	unicode::ConvertU16String(message, native_message);

	nox::assertion::detail::Assert(error_category, native_message.data(), file_name, source_location);
}

void	nox::assertion::detail::Assert(std::u16string_view error_category, std::u16string_view message, const std::wstring_view file_name, const std::source_location& source_location)noexcept(false)
{
	nox::stack_walker::StackWalkerSlim stack_walker;
	stack_walker.Collect(1);

	stack_walker.GetStackList();

	std::array<nox::char16, 4096> assert_message = { 0 };
	nox::util::Format(assert_message, u"{0}\n{1}\nLine:{2}, Column:{3}", message, file_name.data(), source_location.line(), source_location.column());
	
	NOX_LOCAL_SCOPE(os::ScopedLock{ kMutex });
#if! NDEBUG
	::_wassert(nox::util::CharCast<wchar16>(assert_message.data()), file_name.data(), source_location.line());
#endif
}