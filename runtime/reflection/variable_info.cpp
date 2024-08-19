///	@file	field_info.cpp
///	@brief	field_info
#include	"stdafx.h"
#include	"variable_info.h"

#include	"user_defined_compound_type_info.h"
#include	"manager.h"

using namespace nox;
using namespace nox::reflection;

const nox::reflection::UserDefinedCompoundTypeInfo* nox::reflection::VariableInfo::GetContainingUserDefinedCompoundTypeInfo()const noexcept
{
	if (containing_type_ == nox::reflection::InvalidType)
	{
		return nullptr;
	}

	return Reflection::Instance().FindUserDefinedCompoundTypeInfo(containing_type_);
}