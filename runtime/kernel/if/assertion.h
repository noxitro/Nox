///	@file	assertion.h
///	@brief	assertion
#pragma once
#include	"basic_definition.h"
#include	"basic_type.h"

#include	"../type_traits/concepts.h"
#include	"../type_traits/type_traits.h"
#include	"convert_string.h"

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
			void	Assert(not_null<const c16*> message, not_null<const c16*> file, const u32 line);
		}

		inline	void	Assert(const bool expression, not_null<const c16*> message, not_null<const c16*> file = util::CharCast<c16>(__FILEW__), const u32 line = __LINE__)
		{
			if (expression)
			{
				debug::detail::Assert(message, file, line);
			}
		}
	}
}