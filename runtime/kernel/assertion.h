///	@file	assertion.h
///	@brief	assertion
#pragma once
#include	"basic_definition.h"
#include	"basic_type.h"

#include	"type_traits/concepts.h"
#include	"type_traits/type_traits.h"

#include	"preprocessor/util.h"
#include	"string_format.h"
#include	"assertion_id.h"

namespace nox::assertion
{

	namespace detail
	{
		void	Assert(std::u16string_view error_category, std::u8string_view message, const std::wstring_view file_name, const std::source_location& source_location)noexcept(false);
		void	Assert(std::u16string_view error_category, std::u16string_view message, const std::wstring_view file_name, const std::source_location& source_location)noexcept(false);
	}

	template<std::derived_from<nox::assertion::id::ErrorId> Id, class... Args> requires(std::is_invocable_r_v<std::u16string_view, Id>)
	inline void AssertArgs(const std::wstring_view file_name, const std::source_location location, std::u8string_view message, Args&&... args)
	{
		if constexpr (sizeof...(Args) > 0)
		{
			//	動的メモリ確保を行わないように確保済みのバッファを使用
			std::array<nox::char16, 5096> buffer = { 0 };
			nox::util::Format(buffer, message.data(), std::forward<Args>(args)...);

			nox::assertion::detail::Assert(Id()(), buffer.data(), file_name, location);
		}
		else
		{
			nox::assertion::detail::Assert(Id()(), message, file_name, location);
		}
	}

	template<std::derived_from<nox::assertion::id::ErrorId> Id, class... Args> requires(std::is_invocable_r_v<std::u16string_view, Id>)
	inline void AssertArgs(const std::wstring_view file_name, const std::source_location location, std::u16string_view message, Args&&... args)
	{
		if constexpr (sizeof...(Args) > 0)
		{
			//	動的メモリ確保を行わないように確保済みのバッファを使用
			std::array<nox::char16, 5096> buffer = { 0 };
			nox::util::Format(buffer, message.data(), std::forward<Args>(args)...);

			nox::assertion::detail::Assert(Id()(), buffer.data(), file_name, location);
		}
		else
		{
			nox::assertion::detail::Assert(Id()(), message, file_name, location);
		}
	}
}

#if NOX_DEBUG || NOX_RELEASE

#define NOX_ASSERT_ID_IMPL(expression, id, ...) \
	((void)(			\
	(!!(expression)) || \
	(::nox::assertion::AssertArgs<id>(__FILEW__, ::std::source_location::current(), __VA_ARGS__),0))\
	)
//	end define

/// @brief アサート
#define NOX_ASSERT_ID(expression, id, ...) NOX_ASSERT_ID_IMPL(expression, id, __VA_ARGS__)
//	end define
#define	NOX_ASSERT(expression, ...) NOX_ASSERT_ID(expression, nox::assertion::id::Invalid, __VA_ARGS__)

#else
#define	NOX_ASSERT(...) 
#define NOX_ASSERT_ID(...)
#endif // NOX_DEBUG || NOX_RELEASE