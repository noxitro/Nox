///	@file	type.cpp
///	@brief	type
#include	"stdafx.h"
#include	"type.h"
#include	"manager.h"

const class nox::reflection::UserDefinedCompoundTypeInfo* nox::reflection::Type::GetUserDefinedCompoundTypeInfo()const noexcept
{
	return nox::reflection::Reflection::Instance().FindClassInfo(*this);
}

const class nox::reflection::EnumInfo* nox::reflection::Type::GetEnumInfo()const noexcept
{
	return nox::reflection::Reflection::Instance().FindEnumInfo(*this);
}