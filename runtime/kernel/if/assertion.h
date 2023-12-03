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
		enum class RuntimeAssertErrorType : u8
		{
			Other,
		};

		namespace detail
		{
			void	Assert(RuntimeAssertErrorType errorType, not_null<const c16*> message, const std::source_location& source_location)noexcept(false);
		}

		inline	void	Assert(RuntimeAssertErrorType errorType, const bool expression, not_null<const c16*> message, const std::source_location location = std::source_location::current())
		{
			if (!expression)
			{
				debug::detail::Assert(errorType, message, location);
			}
		}

		inline	void	Assert(const bool expression, not_null<const c16*> message, const std::source_location location = std::source_location::current())
		{
			if (!expression)
			{
				debug::detail::Assert(RuntimeAssertErrorType::Other, message, location);
			}
		}
	}
}

#if NOX_DEBUG || NOX_RELEASE
#define	NOX_ASSERT(...) nox::debug::Assert(__VA_ARGS__)
#else
#define	NOX_ASSERT(...) 
#endif // NOX_DEBUG || NOX_RELEASE
