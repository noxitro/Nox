///	@file	field_info.cpp
///	@brief	field_info
#include	"stdafx.h"
#include	"variable_info.h"

#include	"class_info.h"
#include	"database.h"

const nox::reflection::ClassInfo* nox::reflection::VariableInfo::GetContainingUserDefinedCompoundTypeInfo()const noexcept
{
	if (containing_type_ == nox::reflection::GetInvalidType())
	{
		return nullptr;
	}

	return nox::reflection::FindClassInfo(containing_type_);
}