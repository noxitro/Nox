//	Copyright (C) 2024 NOX ENGINE All Rights Rserved.

///	@file	fixed_string.cpp
///	@brief	fixed_string
#include	"stdafx.h"
#include	"fixed_string.h"
#include    "assertion.h"

void nox::detail::CheckStringLength(std::size_t length, std::size_t maxLength)
{
	NOX_ASSERT(length <= maxLength,	u"OutOfRange");
}