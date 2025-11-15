///	@file	assertion.h
///	@brief	assertion
#pragma once
#include	"basic_definition.h"
#include	"basic_type.h"

#include	"type_traits/concepts.h"
#include	"type_traits/type_traits.h"

#include	"preprocessor/util.h"
#include	"string_format.h"

namespace nox::assertion
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
			inline constexpr std::u16string_view operator()() const noexcept { return u"Invalid"; }
		};

		struct NullAccess : ErrorId
		{
			inline constexpr std::u16string_view operator()() const noexcept { return u"NullAccess"; }
		};

		struct OutOfRange : ErrorId
		{
			inline constexpr std::u16string_view operator()() const noexcept { return u"OutOfRange"; }
		};
	}

	namespace detail
	{
		void	Assert(std::u16string_view error_category, std::u32string_view message, const std::wstring_view file_name, const std::source_location& source_location)noexcept(false);
		void	Assert(std::u16string_view error_category, std::u16string_view message, const std::wstring_view file_name, const std::source_location& source_location)noexcept(false);
	}

	inline void Assert(bool expression, std::u32string_view message, const std::wstring_view file_name, const std::source_location location = std::source_location::current())noexcept(false)
	{
		if (!expression)
		{
			assertion::detail::Assert(id::Invalid{}(), message, file_name, location);
		}
	}

	template<std::derived_from<nox::assertion::id::ErrorId> Id> requires(std::is_invocable_r_v<std::u16string_view, Id>)
		inline void Assert(bool expression, Id&& id, std::u16string_view message, const std::wstring_view file_name, const std::source_location location = std::source_location::current())
	{
		if (expression == false)
		{
			nox::assertion::detail::Assert(id(), message, file_name, location);
		}
	}

	inline void Assert(bool expression, std::u16string_view message, const std::wstring_view file_name, const std::source_location location = std::source_location::current())
	{
		if (expression == false)
		{
			nox::assertion::detail::Assert(nox::assertion::id::Invalid{}(), message, file_name, location);
		}
	}

	template<std::derived_from<nox::assertion::id::ErrorId> Id, class... Args> requires(std::is_invocable_r_v<std::u16string_view, Id>)
	inline void AssertArgs(const std::wstring_view file_name, const std::source_location location, bool expression, Id&& id, std::u16string_view message, Args&&... args)
	{
		if (expression == true)
		{
			return;
		}
		else
		{
			if constexpr (sizeof...(Args) > 0)
			{
				//	動的メモリ確保を行わないように確保済みのバッファを使用
				std::array<nox::char16, 5096> buffer = { 0 };
				nox::util::Format(buffer, message.data(), std::forward<Args>(args)...);

				nox::assertion::detail::Assert(id(), buffer.data(), file_name, location);
			}
			else
			{
				nox::assertion::detail::Assert(id(), message, file_name, location);
			}
		}
	}

	template<class... Args>
	inline void AssertArgs(const std::wstring_view file_name, const std::source_location location, bool expression, std::u16string_view message, Args&&... args)
	{
		nox::assertion::AssertArgs(file_name, location, expression, nox::assertion::id::Invalid{}, message, std::forward<Args>(args)...);
	}
}

#if NOX_DEBUG || NOX_RELEASE
/// @brief アサート
#define	NOX_ASSERT(...) \
	::nox::assertion::AssertArgs(__FILEW__, ::std::source_location::current(), __VA_ARGS__)
#define NOX_ASSERT_ID(expression, id, ...) \
	::nox::assertion::AssertArgs<id>(expression, __VA_ARGS__, __FILEW__)
#else
#define	NOX_ASSERT(...) 
#define NOX_ASSERT_ID(...)
#endif // NOX_DEBUG || NOX_RELEASE