///	@file	advanced_definition.cpp
///	@brief	advanced_definition
#include	"pch.h"
#include	"advanced_definition.h"
#include	"assertion.h"
#include	"string_format.h"

void nox::util::detail::AssertDeref(std::string_view name)
{
	NOX_ASSERT(false, u"{0}はnullptrです", name);
}