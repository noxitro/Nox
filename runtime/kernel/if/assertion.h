///	@file	assertion.h
///	@brief	assertion
#pragma once
#include	"basic_definition.h"
#include	"basic_type.h"

#include	"../type_traits/concepts.h"
#include	"../type_traits/type_traits.h"

namespace nox
{
	namespace debug
	{
		enum class RuntimeAssertErrorType : uint8
		{
			Other,
		};

		namespace detail
		{
			void	Assert(RuntimeAssertErrorType errorType, std::u16string_view message, const std::source_location& source_location)noexcept(false);
			void	Assert(RuntimeAssertErrorType errorType, std::u32string_view message, const std::source_location& source_location)noexcept(false);
		}

		inline	void	Assert(RuntimeAssertErrorType errorType, std::u16string_view message, const std::source_location location = std::source_location::current())noexcept(false)
		{
			debug::detail::Assert(errorType, message, location);
		}

		inline	void	Assert(std::u16string_view message, const std::source_location location = std::source_location::current())noexcept(false)
		{
			debug::detail::Assert(RuntimeAssertErrorType::Other, message, location);
		}

		inline	void	Assert(std::u32string_view message, const std::source_location location = std::source_location::current())noexcept(false)
		{
			debug::detail::Assert(RuntimeAssertErrorType::Other, message, location);
		}
	}
}

#if NOX_DEBUG || NOX_RELEASE
#define	NOX_ASSERT(expression, ...) \
	static_assert(std::is_same_v<decltype(expression), bool>, "expression is not bool"); \
	(void)((!!(expression)) || (::nox::debug::Assert(__VA_ARGS__), 0)); \
	__analysis_assume(expression)
#else
#define	NOX_ASSERT(...) 
#endif // NOX_DEBUG || NOX_RELEASE
