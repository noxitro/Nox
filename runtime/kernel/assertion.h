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
		namespace id
		{
			/// @brief ログID
			/// @details ログIDを定義する構造体を継承することで、ログIDを定義できます
			struct ErrorId
			{
			};

			/// @brief 無効なログID
			struct Invalid : ErrorId
			{
				inline constexpr std::u32string_view operator()() const noexcept { return U"Invalid"; }
			};

			struct NullAccess : ErrorId
			{
				inline constexpr std::u32string_view operator()() const noexcept { return U"NullAccess"; }
			};

			struct OutOfRange : ErrorId
			{
				inline constexpr std::u32string_view operator()() const noexcept { return U"OutOfRange"; }
			};
		}

		namespace detail
		{
			void	Assert(std::u32string_view error_category, std::u32string_view message, const std::wstring_view file_name, const std::source_location& source_location)noexcept(false);
		}

		template<std::derived_from<id::ErrorId> ErrorId>
			requires(std::is_same_v<std::u32string_view, decltype(ErrorId()())>)
		inline	void	Assert(std::u32string_view message, const std::wstring_view file_name, const std::source_location location = std::source_location::current())noexcept(false)
		{
			assertion::detail::Assert(ErrorId{}(), message, file_name, location);
		}

		inline	void	Assert(std::u32string_view message, const std::wstring_view file_name, const std::source_location location = std::source_location::current())noexcept(false)
		{
			assertion::detail::Assert(id::Invalid{}(), message, file_name, location);
		}

		inline void Assert(bool expression, std::u32string_view message, const std::wstring_view file_name, const std::source_location location = std::source_location::current())noexcept(false)
		{
			if (!expression)
			{
				assertion::detail::Assert(id::Invalid{}(), message, file_name, location);
			}
		}

		template<std::derived_from<id::ErrorId> ErrorId>
			requires(std::is_same_v<std::u32string_view, decltype(ErrorId()())>)
		inline void Assert(bool expression, std::u32string_view message, const std::wstring_view file_name, const std::source_location location = std::source_location::current())noexcept(false)
		{
			if (!expression)
			{
				assertion::detail::Assert(ErrorId{}(), message, file_name, location);
			}
		}
	}
}

#if NOX_DEBUG || NOX_RELEASE
/// @brief アサート
#define	NOX_ASSERT(...) \
	::nox::assertion::Assert(__VA_ARGS__, __FILEW__)
#define NOX_ASSERT_ID(expression, id, ...) \
	::nox::assertion::Assert<id>(expression, __VA_ARGS__, __FILEW__)
#else
#define	NOX_ASSERT(...) 
#define NOX_ASSERT_ID(...)
#endif // NOX_DEBUG || NOX_RELEASE