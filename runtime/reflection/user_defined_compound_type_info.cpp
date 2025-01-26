///	@file	user_defined_compound_type_info.cpp
///	@brief	user_defined_compound_type_info
#include	"stdafx.h"
#include	"user_defined_compound_type_info.h"

#include	"function_info.h"
#include	"variable_info.h"

#include	"database.h"
bool	nox::reflection::ClassInfo::IsBaseOf(const nox::reflection::ClassInfo& derived)const noexcept
{
	return nox::reflection::IsBaseOf(*this, derived);
}