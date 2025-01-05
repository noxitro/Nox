///	@file	type.cpp
///	@brief	type
#include	"stdafx.h"
#include	"type.h"

#include	"database.h"

const class nox::reflection::UserDefinedCompoundTypeInfo* nox::reflection::Type::GetUserDefinedCompoundTypeInfo()const noexcept
{
	return nox::reflection::FindClassInfo(*this);
}

const class nox::reflection::EnumInfo* nox::reflection::Type::GetEnumInfo()const noexcept
{
	return nox::reflection::FindEnumInfo(*this);
}