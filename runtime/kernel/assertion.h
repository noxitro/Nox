///	@file	assertion.h
///	@brief	assertion
#pragma once
#include	"basic_definition.h"
#include	"basic_type.h"

#include	"type_traits/concepts.h"
#include	"type_traits/type_traits.h"

#include	"preprocessor/util.h"

namespace nox
{
	namespace assertion
	{
		enum class RuntimeAssertErrorType : uint8
		{
			Default,
			NullAccess,

			/// @brief 範囲外アクセス
			OutOfRange,
			_MAX
		};

		namespace detail
		{
			void	Assert(RuntimeAssertErrorType errorType, std::u32string_view message, const std::wstring_view file_name, const std::source_location& source_location)noexcept(false);
		}

		inline	void	Assert(RuntimeAssertErrorType errorType, std::u32string_view message, const std::wstring_view file_name, const std::source_location location = std::source_location::current())noexcept(false)
		{
			assertion::detail::Assert(errorType, message, file_name, location);
		}

		inline	void	Assert(std::u32string_view message, const std::wstring_view file_name, const std::source_location location = std::source_location::current())noexcept(false)
		{
			assertion::detail::Assert(RuntimeAssertErrorType::Default, message, file_name, location);
		}

		inline void Assert(bool expression, std::u32string_view message, const std::wstring_view file_name, const std::source_location location = std::source_location::current())noexcept(false)
		{
			if (!expression)
			{
				assertion::detail::Assert(RuntimeAssertErrorType::Default, message, file_name, location);
			}
		}

		inline void Assert(bool expression, RuntimeAssertErrorType error_type, std::u32string_view message, const std::wstring_view file_name, const std::source_location location = std::source_location::current())noexcept(false)
		{
			if (!expression)
			{
				assertion::detail::Assert(error_type, message, file_name, location);
			}
		}
	}
}

#if NOX_DEBUG || NOX_RELEASE
/// @brief アサート
#define	NOX_ASSERT(...) \
	::nox::assertion::Assert(__VA_ARGS__, __FILEW__)
#else
#define	NOX_ASSERT(...) 
#endif // NOX_DEBUG || NOX_RELEASE