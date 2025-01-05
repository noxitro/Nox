//	Copyright (C) 2024 NOX ENGINE All Rights Rserved.

///	@file	string.cpp
///	@brief	string
#include	"stdafx.h"
#include	"nox_string.h"

#include	"nox_string_view.h"

nox::String::String(class nox::StringView other)noexcept:
	String(std::u32string_view(other))
{

}
